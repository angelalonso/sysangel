#!/usr/bin/env python3
import time
import sys
import pygame
import numpy as np

def play_beep(frequency=800, duration=0.3, volume=0.3):
    """Play a beep sound using pygame"""
    try:
        # Initialize pygame mixer if not already initialized
        if not pygame.mixer.get_init():
            pygame.mixer.init(frequency=44100, size=-16, channels=2)
        
        sample_rate = 44100
        n_samples = int(round(duration * sample_rate))
        
        # Generate samples for a sine wave
        buf = np.zeros((n_samples, 2), dtype=np.int16)
        max_sample = 2**(16 - 1) - 1
        
        for s in range(n_samples):
            t = float(s) / sample_rate
            sample = max_sample * volume * np.sin(2 * np.pi * frequency * t)
            buf[s][0] = int(sample)
            buf[s][1] = int(sample)
        
        # Create and play sound
        sound = pygame.sndarray.make_sound(buf)
        sound.play()
        
        # Wait for sound to finish
        pygame.time.wait(int(duration * 1000))
        
    except Exception as e:
        print(f"Error playing sound: {e}")
        # Fallback to system beep
        sys.stdout.write('\a')
        sys.stdout.flush()

def countdown_to_start(duration=5):
    """Countdown with visual display before timer starts"""
    print(f"\nStarting in {duration} seconds...")
    
    for i in range(duration, 0, -1):
        sys.stdout.write(f"\r{i:2d}... ")
        sys.stdout.flush()
        
        # Play ticking sound for countdown
        if i <= 3:  # Only beep for last 3 seconds
            play_beep(frequency=600, duration=0.1, volume=0.3)
        
        time.sleep(1)
    
    print("\rGO!   ")
    return time.time()  # Return exact start time

def startup_beeps():
    """Play 3 quick beeps to indicate timer is starting"""
    print("\n" + "═" * 40)
    print("STARTUP SEQUENCE - 3 warning beeps")
    print("Timer will run for 20 cycles (4 minutes)")
    print("═" * 40)
    
    # Three warning beeps
    for i in range(3):
        play_beep(frequency=1000, duration=0.2, volume=0.4)
        time.sleep(0.2)
    
    print("Timer will start in 5 seconds...")

def precise_countdown():
    """Countdown timer with exactly 12 seconds between beeps"""
    print("12-Second Countdown Timer (20 cycles)")
    print("=" * 50)
    
    # Initialize pygame
    pygame.init()
    
    # Play startup beeps
    startup_beeps()
    
    # 5-second countdown before timer starts
    exact_start_time = countdown_to_start(5)
    
    print("\n" + "★" * 50)
    print("TIMER ACTIVE! First 12-second interval has begun")
    print(f"Will stop after 20 cycles (4 minutes total)")
    print("★" * 50 + "\n")
    
    try:
        # Calculate first beep time (12 seconds after exact start)
        next_beep_time = exact_start_time + 12
        cycle_count = 1
        max_cycles = 20
        
        while cycle_count <= max_cycles:
            # Display countdown to next beep with progress
            while True:
                time_until_beep = next_beep_time - time.time()
                
                if time_until_beep <= 0:
                    break
                
                # Format time display
                mins, secs = divmod(int(time_until_beep), 60)
                secs_frac = int((time_until_beep - int(time_until_beep)) * 10)
                
                # Display with cycle number and progress
                progress = f"{cycle_count}/{max_cycles}"
                sys.stdout.write(f"\rCycle {progress:>7}: {mins:02d}:{secs:02d}.{secs_frac}s until beep   ")
                sys.stdout.flush()
                
                # Sleep in small increments for responsiveness
                sleep_time = min(0.1, time_until_beep)
                time.sleep(sleep_time)
            
            # Play beep at exact time
            print(f"\rCycle {cycle_count:3d}/{max_cycles}: BEEP! {time.strftime('%H:%M:%S')}           ")
            play_beep(frequency=800, duration=0.4, volume=0.5)
            
            # If this was the last cycle, exit
            if cycle_count == max_cycles:
                break
            
            # Schedule next beep exactly 12 seconds after current one
            next_beep_time += 12
            cycle_count += 1
            
        # Timer completed all 20 cycles
        print("\n" + "=" * 60)
        print("TIMER COMPLETED!")
        print(f"Finished {max_cycles} cycles ({max_cycles * 12} seconds total)")
        print("=" * 60)
        
        # Play completion sound sequence
        print("\nPlaying completion signal...")
        for i in range(3):
            play_beep(frequency=600, duration=0.2, volume=0.4)
            time.sleep(0.15)
        
        # Final long beep
        play_beep(frequency=400, duration=1.0, volume=0.5)
        
    except KeyboardInterrupt:
        print("\n\n" + "=" * 50)
        print(f"Timer stopped manually after {cycle_count-1} cycles")
        print("=" * 50)
    
    finally:
        pygame.quit()
        sys.exit(0)

if __name__ == "__main__":
    precise_countdown()
