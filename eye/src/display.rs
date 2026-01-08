use std::io::{self, Write};
use crate::config::ScreenConfig;
use crate::mouse::MouseState;

pub struct Display {
    screen: ScreenConfig,
}

impl Display {
    pub fn new(screen: ScreenConfig) -> Self {
        Display { screen }
    }
    
    pub fn update(&self, state: &MouseState) {
        // ONLY this line should print: "X - Y (WxH)"
        print!("\r{} - {} ({}x{})", 
            state.x, 
            state.y, 
            self.screen.width, 
            self.screen.height
        );
        io::stdout().flush().unwrap();
    }
}
