#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]
use once_cell::sync::Lazy;
use std::sync::Mutex;
use std::process::{Command, Stdio};
use std::io::{Write, BufRead, BufReader};
use tauri::{Emitter, AppHandle}; // ⭐ Import Emitter to send events

struct EngineProcess {
    stdin: std::process::ChildStdin,
}

static ENGINE: Lazy<Mutex<Option<EngineProcess>>> = Lazy::new(|| Mutex::new(None));

// ⭐ Accept app_handle to send events to the frontend
fn start_engine(app_handle: AppHandle) -> Result<(), String> {
    let mut engine_lock = ENGINE.lock().unwrap();
    if engine_lock.is_some() {
        return Ok(()); 
    }

    eprintln!("STARTING AI_SHELL.EXE PROCESS..."); 
    
    let exe_path = "C:\\Users\\Bobio\\source\\ai_shell\\x64\\Debug\\ai_shell.exe";
    let exe_dir = "C:\\Users\\Bobio\\source\\ai_shell\\x64\\Debug";

    let mut child = Command::new(exe_path)
        .current_dir(exe_dir) 
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|e| format!("Process spawn failed: {}", e))?;

    let stdin = child.stdin.take().ok_or("Failed to map stdin")?;
    let stdout = child.stdout.take().ok_or("Failed to map stdout")?;
    let stderr = child.stderr.take().ok_or("Failed to map stderr")?;

    // 🧵 Thread 1: Read stdout and EMIT to frontend
    std::thread::spawn(move || {
        let reader = BufReader::new(stdout);
        for line in reader.lines() {
            if let Ok(msg) = line {
                eprintln!("[AI_SHELL STDOUT] {}", msg);
                // ⭐ Emit event named "engine-output" to React
                let _ = app_handle.emit("engine-output", msg);
            }
        }
    });

    // 🧵 Thread 2: Read stderr continuously
    std::thread::spawn(move || {
        let reader = BufReader::new(stderr);
        for line in reader.lines() {
            if let Ok(msg) = line {
                eprintln!("[AI_SHELL STDERR] {}", msg);
            }
        }
    });

    *engine_lock = Some(EngineProcess { stdin });
    Ok(())
}

#[tauri::command]
async fn load_model(app_handle: AppHandle, path: String) -> Result<String, String> {
    eprintln!("LOAD_MODEL EXECUTION PATH: {}", path);
    
    start_engine(app_handle)?; // Pass handle down

    tokio::task::spawn_blocking(move || {
        let mut engine = ENGINE.lock().unwrap();
        let engine = engine.as_mut().unwrap();

        let command = format!("OPEN default {}\n", path);
        engine.stdin.write_all(command.as_bytes()).map_err(|e| e.to_string())?;
        engine.stdin.flush().map_err(|e| e.to_string())?;

        Ok("Model command sent".to_string())
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

        // ⭐ Return an empty string or status so the frontend ignores this value
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
