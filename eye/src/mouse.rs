use std::fs::File;
use std::io::{self, Read};
use std::process::Command;

use crate::config::ScreenConfig;

#[derive(Debug, Clone, Copy)]
pub struct MouseState {
    pub x: i32,
    pub y: i32,
}

impl MouseState {
    pub fn new(screen: &ScreenConfig) -> Self {
        let (mut x, mut y) = (screen.width / 2, screen.height / 2);
        
        // Try to get actual current position
        if let Ok(output) = Command::new("xdotool").arg("getmouselocation").output() {
            if let Ok(stdout) = String::from_utf8(output.stdout) {
                for part in stdout.split_whitespace() {
                    if part.starts_with("x:") {
                        x = part[2..].parse().unwrap_or(x);
                    } else if part.starts_with("y:") {
                        y = part[2..].parse().unwrap_or(y);
                    }
                }
            }
        }
        
        MouseState { x, y }
    }
    
    pub fn update(&mut self, dx: i32, dy: i32, screen: &ScreenConfig) {
        // Update position with screen bounds
        self.x = (self.x + dx).max(0).min(screen.width - 1);
        self.y = (self.y + dy).max(0).min(screen.height - 1);
    }
}

pub struct MouseDevice {
    file: File,
}

impl MouseDevice {
    pub fn open(path: &str) -> io::Result<Self> {
        let file = File::open(path)?;
        Ok(MouseDevice { file })
    }
    
    pub fn read_packet(&mut self) -> io::Result<Option<(i32, i32)>> {
        let mut buffer = [0u8; 3];
        
        match self.file.read_exact(&mut buffer) {
            Ok(_) => {
                // Decode PS/2 mouse packet
                let dx = buffer[1] as i8 as i32;
                let dy = buffer[2] as i8 as i32;
                Ok(Some((dx, dy)))
            }
            Err(e) if e.kind() == io::ErrorKind::UnexpectedEof => Ok(None),
            Err(e) => Err(e),
        }
    }
}
