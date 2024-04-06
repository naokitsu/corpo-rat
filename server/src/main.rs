use std::io::{Error, ErrorKind, Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;
use std::time::Duration;

fn handle_client(mut stream: TcpStream) -> Result<(), Error> {
    let mut buffer = [0; 1024];
    let message = &[1u8];

    loop {
        stream.write_all(message)?;
        thread::sleep(Duration::from_secs(1));

        let size = stream.read(&mut buffer)?;
        if size == 0 {
            break Ok(());
        }

        println!("Received: {:?}", &buffer[..size]);
    }
}

fn main() -> Result<(), Error> {
    let address = "127.0.0.1:4444"; // Replace with your desired address
    let listener = TcpListener::bind(address)?;

    println!("Listening on {}", address);

    loop {
        let (mut stream, _) = listener.accept()?;
        println!("New connection!");

        // Spawn a thread to handle each client separately
        thread::spawn(move || {
            if let Err(err) = handle_client(stream) {
                if err.kind() != ErrorKind::BrokenPipe {
                    eprintln!("Error handling client: {}", err);
                }
            }
        });
    }
}