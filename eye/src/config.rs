use std::process::Command;

#[derive(Debug, Clone, Copy)]
pub struct ScreenConfig {
    pub width: i32,
    pub height: i32,
}

impl ScreenConfig {
    pub fn get() -> Self {
        // Try to get resolution from xrandr
        if let Ok(output) = Command::new("xrandr").output() {
            if let Ok(stdout) = String::from_utf8(output.stdout) {
                for line in stdout.lines() {
                    if line.contains('*') && line.contains('+') {
                        for word in line.split_whitespace() {
                            if word.contains('x') {
                                let parts: Vec<&str> = word.split('x').collect();
                                if parts.len() == 2 {
                                    if let (Ok(w), Ok(h)) = (parts[0].parse(), parts[1].parse()) {
                                        return ScreenConfig { width: w, height: h };
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        // Fallback to 1080p
        ScreenConfig {
            width: 1920,
            height: 1080,
        }
    }
}
