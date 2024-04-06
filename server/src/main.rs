use std::fs::File;
use std::io::{Error, ErrorKind, Read, Write};
use std::net::{TcpListener, TcpStream};
use std::thread;
use std::time::Duration;

fn handle_client(mut stream: TcpStream) -> Result<(), Error> {
    //let mut buffer = [0; 1024];
    let message = &[2u8];
    let mut img_size: [u8; 8] = [0; 8];
    stream.write_all(message)?;

    let _ = stream.read_exact(&mut img_size);
    let img_size: u64 =
        (img_size[3] as u64 ) << 24
            | (img_size[2] as u64) << 16
            | (img_size[1] as u64) << 8
            | (img_size[0] as u64);

    let mut buffer = vec![0u8; img_size as usize];
    stream.read_exact(&mut buffer)?;
    let mut file = File::create("foo.jpg")?;
    file.write_all(buffer.as_slice())?;

    loop {
        
    }

    Ok(())
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