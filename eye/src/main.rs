mod config;
mod mouse;
mod display;

use std::env;
use config::ScreenConfig;
use mouse::{MouseState, MouseDevice};
use display::Display;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // Get device path from args (default to mouse2)
    let device_path = env::args()
        .nth(1)
        .unwrap_or_else(|| "/dev/input/mouse2".to_string());
    
    // Initialize components
    let screen = ScreenConfig::get();
    let mut state = MouseState::new(&screen);
    let mut device = MouseDevice::open(&device_path)?;
    let display = Display::new(screen);
    
    // Show initial position
    display.update(&state);
    
    // Main tracking loop
    loop {
        match device.read_packet() {
            Ok(Some((dx, dy))) => {
                state.update(dx, dy, &screen);
                display.update(&state);
            }
            Ok(None) => {
                // No data available
                continue;
            }
            Err(e) => {
                eprintln!("\nError: {}", e);
                break;
            }
        }
    }
    
    println!(); // New line after Ctrl+C
    Ok(())
}
