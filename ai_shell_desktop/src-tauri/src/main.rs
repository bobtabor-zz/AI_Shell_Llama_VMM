#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use once_cell::sync::Lazy;
use std::sync::Mutex;
use std::process::{Command, Stdio, Child, ChildStdin}; // 1. Added ChildStdin
use std::io::{Write, BufRead, BufReader};
use tauri::{Emitter, AppHandle};
use std::os::windows::process::CommandExt; 

struct EngineProcess {
    // We keep both so your other commands can write to stdin, 
    // and our setup function can cleanly kill the process.
    stdin: ChildStdin,
    child_handle: Child,
}

static ENGINE: Lazy<Mutex<Option<EngineProcess>>> = Lazy::new(|| Mutex::new(None));

fn start_engine(app_handle: AppHandle) -> Result<(), String> {
    let mut engine_lock = ENGINE.lock().unwrap();
    
    // Explicitly kill the old process instead of just dropping stdin
    if let Some(mut active_process) = engine_lock.take() {
        eprintln!("💥 FORCING UNLOAD: Terminating existing ai_shell.exe process...");
        let _ = active_process.child_handle.kill();
        let _ = active_process.child_handle.wait(); // Prevent zombie processes
    }

    // 🌐 AUTOMATION STEP 1: Force kill any active instances of Chrome
    eprintln!("🧹 TERMINATING CURRENT CHROME INSTANCES...");
    let _ = Command::new("taskkill")
        .args(&["/IM", "chrome.exe", "/F"])
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .status();

    // 🌐 AUTOMATION STEP 2: Launch Chrome with Remote Debugging flags
    eprintln!("🌐 LAUNCHING CHROME IN REMOTE DEBUGGING MODE...");
    
    // Windows flags: 0x00000008 (Detached) + 0x08000000 (No Window)
    const DETACHED_PROCESS: u32 = 0x00000008;
    const CREATE_NO_WINDOW: u32 = 0x08000000;
    const FLAGS: u32 = DETACHED_PROCESS | CREATE_NO_WINDOW;

    Command::new("C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe")
        .args(&[
            "--remote-debugging-port=9222",
            "--user-data-dir=C:\\ChromeDebug",
            "--no-first-run",
            "--no-default-browser-check",
        ])
        .stdout(Stdio::null())
        .stderr(Stdio::null())
        .stdin(Stdio::null())
        .creation_flags(FLAGS) 
        .spawn()
        .map_err(|e| format!("Failed to launch Chrome: {}", e))?;

    // Pause to let Chrome bind to port 9222
    std::thread::sleep(std::time::Duration::from_millis(1500));

    eprintln!("🚀 SPAWNING FRESH AI_SHELL.EXE PROCESS...");
    let exe_path = "C:\\Users\\Bobio\\source\\ai_shell\\x64\\Debug\\ai_shell.exe";
    let exe_dir = "C:\\Users\\Bobio\\source\\ai_shell\\x64\\Debug";

    let mut child = Command::new(exe_path)
        .current_dir(exe_dir)
        .env("LLAMA_NO_MMAP", "1")
        .env("LLAMA_VMM_BUILD", "1")
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|e| format!("Process spawn failed: {}", e))?;

    // 2. Extract stdin, stdout, and stderr properly
    let stdin = child.stdin.take().ok_or("Failed to map stdin")?;
    let stdout = child.stdout.take().ok_or("Failed to map stdout")?;
    let stderr = child.stderr.take().ok_or("Failed to map stderr")?;

    // Monitor STDOUT
    std::thread::spawn(move || {
        let reader = BufReader::new(stdout);
        for line in reader.lines() {
            if let Ok(msg) = line {
                eprintln!("[AI_SHELL STDOUT] {}", msg);
                let _ = app_handle.emit("engine-output", msg);
            }
        }
    });

    // Monitor STDERR
    std::thread::spawn(move || {
        let reader = BufReader::new(stderr);
        for line in reader.lines() {
            if let Ok(msg) = line {
                eprintln!("[AI_SHELL STDERR] {}", msg);
            }
        }
    });

    // 3. Save both the extracted stdin and child handle to global state
    *engine_lock = Some(EngineProcess { 
        stdin,
        child_handle: child 
    });
    
    Ok(())
}

#[tauri::command] 
async fn load_model(app_handle: AppHandle, path: String) -> Result<String, String> { 
    eprintln!("🔄 MODEL HOT-SWAP ROUTINE INITIATED FOR: {}", path); 
    start_engine(app_handle)?; 
    tokio::task::spawn_blocking(move || { 
        let mut engine = ENGINE.lock().unwrap(); 
        let engine = engine.as_mut().unwrap(); 
        eprintln!("➡%EF%B8%8F PINGING ENGINE SYSTEM: UNLOAD"); 
        engine.stdin.write_all(b"UNLOAD\n").map_err(|e| format!("UNLOAD write failed: {}", e))?; 
        engine.stdin.flush().map_err(|e| format!("UNLOAD flush failed: {}", e))?; 
        std::thread::sleep(std::time::Duration::from_millis(500)); 
        eprintln!("➡%EF%B8%8F PINGING ENGINE SYSTEM: OPEN -> {}", path); 
        let command = format!("OPEN default {}\n", path); 
        engine.stdin.write_all(command.as_bytes()).map_err(|e| format!("OPEN write failed: {}", e))?; 
        engine.stdin.flush().map_err(|e| format!("OPEN flush failed: {}", e))?; 
        Ok(format!("Engine model swapped to: {}", path)) 
    }) 
    .await 
    .unwrap() 
} 

#[tauri::command] 
async fn chat(prompt: String) -> Result<String, String> { 
    tokio::task::spawn_blocking(move || { 
        let mut engine = ENGINE.lock().unwrap(); 
        let engine = engine.as_mut().ok_or("Engine process not active")?; 
        let command = format!("CHAT {}\n", prompt); 
        engine.stdin.write_all(command.as_bytes()).map_err(|e| e.to_string())?; 
        engine.stdin.flush().map_err(|e| e.to_string())?; 
        Ok("OK".to_string()) 
    }) 
    .await 
    .unwrap() 
} 

fn main() { 
    tauri::Builder::default() 
        .invoke_handler(tauri::generate_handler![load_model, chat]) 
        .run(tauri::generate_context!()) 
        .expect("Runtime panic inside Tauri layout"); 
}
