#![feature(str_from_utf16_endian)]
#[macro_use] extern crate rocket;
extern crate tera;

use std::collections::HashMap;
use std::fs::File;
use std::io::{Cursor, ErrorKind, Read, Write};
use std::net::{Ipv4Addr, SocketAddr, SocketAddrV4, TcpListener, TcpStream};
use std::sync::Arc;
use std::{env, thread};
use std::time::{Duration, SystemTime};
use chrono::{DateTime, Utc};
use dashmap::DashMap;
use lazy_static::lazy_static;
use rocket::http::{ContentType, Status};
use rocket::response::content::RawHtml;
use rocket::serde::Serialize;
use rocket::{Request, Response, State};
use rocket::response::Responder;
use rocket::tokio::io::split;
use tera::{Context, Tera};


lazy_static! {
    pub static ref TEMPLATES: Tera = {
        let mut tera = match Tera::new("templates/**/*") {
            Ok(t) => t,
            Err(e) => {
                println!("Parsing error(s): {}", e);
                ::std::process::exit(1);
            }
        };
        tera.autoescape_on(vec![".html"]);
        tera
    };
}

struct Image(Vec<u8>);

impl<'a> Responder<'a, 'a> for Image {
    fn respond_to(self, _: &Request) -> rocket::response::Result<'a> {
        Response::build()
            .header(ContentType::JPEG)
            .status(Status::Ok)
            .sized_body(self.0.len(), Cursor::new(self.0))
            .ok()
    }
}

#[get("/image/<domain>/<machine>/<user>")]
async fn get_image(domain: String, machine: String, user: String, sessions: &State<Arc<DashMap<Id, Client>>>) -> Result<Image, ()> {
    let mut session = &sessions.get(&Id {user, machine, domain});
    let mut stream = &if let Some(res) = session {
        res
    } else {
        return Err(())
    }.stream;

    let message = &[2u8];
    let mut img_size: [u8; 8] = [0; 8];
    stream.write_all(message).expect("test");

    let _ = stream.read_exact(&mut img_size);
    let img_size: u64 =
        (img_size[7] as u64 ) << 8 * 7
            | (img_size[6] as u64) << 8 * 6
            | (img_size[5] as u64) << 8 * 5
            | (img_size[4] as u64) << 8 * 4
            | (img_size[3] as u64) << 8 * 3
            | (img_size[2] as u64) << 8 * 2
            | (img_size[1] as u64) << 8 * 1
            | (img_size[0] as u64) << 8 * 0;

    let mut buffer = vec![0u8; img_size as usize];
    stream.read_exact(&mut buffer).expect("test");
    Ok(Image(buffer))
}

#[get("/")]
async fn index(sessions: &State<Arc<DashMap<Id, Client>>>) -> RawHtml<String> {
    #[derive(Serialize)]
    struct DisplayedClient {
        ip: String,
        user: String,
        machine: String,
        domain: String,
        time: String,
    };

    sessions
        .iter_mut()
        .for_each(|mut x| {
            let message = &[3u8]; // activity
            let mut stream = &x.value().stream;
            let mut buf = [0u8; 4];
            x.value_mut().active = Ok::<_, ()>(stream)
                .and_then(|mut stream| {
                    stream.write_all(message).map_err(|_|())?;
                    Ok(stream)
                })
                .and_then(|mut stream| {
                    let mut buf = [0u8; 4];
                    stream.read_exact(&mut buf).map_err(|_|())?;
                    let seconds: u64 =
                        (buf[3] as u64 ) << 8 * 3
                            | (buf[2] as u64) << 8 * 2
                            | (buf[1] as u64) << 8 * 1
                            | (buf[0] as u64) << 8 * 0;

                    Ok(Duration::from_secs(seconds))
                })
                .ok();
        });

    sessions
        .retain(|x, y| y.active != None);

    let now = SystemTime::now();

    let data = sessions
        .iter()
        .map(|x| {
            let (id, client) = x.pair();
            let absolute_time = now - client.active.unwrap_or(Duration::new(0, 0));
            let datetime: DateTime<Utc> = DateTime::from(absolute_time);
            DisplayedClient {
                ip: client.stream.peer_addr().map(|x| x.to_string()).unwrap_or("Unknown".to_string()),
                user: id.user.clone(),
                machine: id.machine.clone(),
                domain: id.domain.clone(),
                time: datetime.format("%d/%m/%Y %T").to_string()
            }
        })
        .collect::<Vec<DisplayedClient>>();

    let mut ctx = Context::new();
    ctx.insert("sessions", &data);
    let html = TEMPLATES.render("index.html", &ctx)
        .expect("Unable to render html document, check the template file");
    RawHtml(html)
}

#[derive(Ord, PartialOrd, Eq, PartialEq, Hash, Debug)]
struct Id {
    user: String,
    machine: String,
    domain: String
}



#[derive(Debug)]
struct Client {
    stream: TcpStream,
    active: Option<Duration>,
}

#[launch]
fn rocket() -> _ {
    let mut args = env::args();
    args.next();

    let ip = Ipv4Addr::from(args.next().unwrap_or("0.0.0.0".to_string()).parse().unwrap_or(Ipv4Addr::new(0,0,0,0)));
    let address = SocketAddrV4::new(ip, args.next().unwrap_or("4444".to_string()).parse().unwrap_or(4444));
    let listener = TcpListener::bind(address)
        .expect("Couldn't bind listener");

    let mut sessions: Arc<DashMap<Id, Client>> = Arc::new(DashMap::new());

    let cloned = sessions.clone();
    thread::spawn(move || {
        loop {
            let (mut stream, _) = listener.accept()
                .expect("Couldn't establish connection");

            let message = [1u8]; // kInfo
            let mut size = [0u8; 4];
            if let Err(x) = stream.write_all(&message) {
                continue;

            };

            if let Err(x) = stream.read_exact(&mut size) {
                continue;
            }

            let size: u64 =
                (size[3] as u64 ) << 8 * 3
                    | (size[2] as u64) << 8 * 2
                    | (size[1] as u64) << 8 * 1
                    | (size[0] as u64) << 8 * 0;

            let mut buffer = vec![0u8; size as usize];

            if let Err(x) = stream.read_exact(&mut buffer) {
                continue;
            };

            let res = if let Ok(x) = String::from_utf8(buffer) {
                x
            } else {
                continue;
            };
            
            let count = res.chars().filter(|x| *x == '.').count();
            let mut split = res.split_terminator('.');
            let (user, machine, domain) = match count {
                2 => {
                    let machine = split.next().unwrap_or("Unknown");
                    let domain = split.next().unwrap_or("Unknown");
                    let user = split.next().unwrap_or("Unknown");
                    (user, machine, domain)
                }
                1 => {
                    let machine = split.next().unwrap_or("Unknown");
                    let domain = "Unknown";
                    let user = split.next().unwrap_or("Unknown");
                    (user, machine, domain)
                }
                _ =>{
                    continue;
                }
            };

            sessions.insert(Id {
                user: user.to_string(),
                machine: machine.to_string(),
                domain: domain.to_string(),
            }, Client {
                stream,
                active: Some(Duration::new(0,0))
            });
        }
    });

    rocket::build()
        .manage(cloned)
        .mount("/", routes![index, get_image])
}