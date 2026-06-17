#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use once_cell::sync::Lazy;
use std::sync::Mutex;
use std::process::{Command, Stdio};
use std::io::{Write, BufRead, BufReader};

struct EngineProcess {
    child: std::process::Child,
    stdin: std::process::ChildStdin,
    stdout: BufReader<std::process::ChildStdout>,
}

static ENGINE: Lazy<Mutex<Option<EngineProcess>>> = Lazy::new(|| Mutex::new(None));

fn start_engine() -> Result<(), String> {
     println!("START_ENGINE CALLED");
    let mut child = Command::new("C:\\Users\\btabor\\source\\ai_shell\\x64\\Debug\\ai_shell.exe")
        .stdin(Stdio::piped())
        .stdout(Stdio::piped())
        .spawn()
        //.map_err(|e| e.to_string())?;
        .map_err(|e| {
    println!("FAILED TO START ENGINE: {}", e);
    e.to_string()})?;

    let stdin = child.stdin.take().ok_or("no stdin")?;
    let stdout = BufReader::new(child.stdout.take().ok_or("no stdout")?);

    *ENGINE.lock().unwrap() = Some(EngineProcess { child, stdin, stdout });

    Ok(())
}

#[tauri::command]
async fn load_model(path: String) -> Result<String, String> {
    println!("LOAD_MODEL CALLED WITH PATH: {}", path);
    tokio::task::spawn_blocking(move || {
        {
            let mut engine = ENGINE.lock().unwrap();
            if engine.is_none() {
                start_engine()?;
            }
        }

        let mut engine = ENGINE.lock().unwrap();
        let engine = engine.as_mut().unwrap();

        // ⭐ SEND THE OPEN COMMAND EXACTLY LIKE THE CONSOLE
        writeln!(engine.stdin, "OPEN default {}", path)
            .map_err(|e| e.to_string())?;

        // ⭐ READ ONE LINE OF RESPONSE (e.g., "MODEL LOADED OK")
        let mut output = String::new();
        engine.stdout.read_line(&mut output)
            .map_err(|e| e.to_string())?;

        Ok(output)
    })
    .await
    .unwrap()
}

#[tauri::command]
async fn chat(prompt: String) -> Result<String, String> {
    tokio::task::spawn_blocking(move || {
        let mut engine = ENGINE.lock().unwrap();
        let engine = engine.as_mut().ok_or("engine not started")?;

        // ⭐ SEND CHAT COMMAND
        writeln!(engine.stdin, "CHAT {}", prompt)
            .map_err(|e| e.to_string())?;

        // ⭐ READ ONE LINE OF RESPONSE
        let mut output = String::new();
        engine.stdout.read_line(&mut output)
            .map_err(|e| e.to_string())?;

        Ok(output)
    })
    .await
    .unwrap()
}

fn main() {
    tauri::Builder::default()
        .invoke_handler(tauri::generate_handler![load_model, chat])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}
