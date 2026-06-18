import { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import Sidebar from "./components/Sidebar";
import ChatWindow from "./components/ChatWindow";
import "./styles/App.css";

// 🛡️ GLOBAL FLAGS: Outlives React component mount/unmount cycles
let isListenerRegistered = false;

export default function App() {
    const [sessionId, setSessionId] = useState(Date.now().toString());
    const [messages, setMessages] = useState([]);
    const hasLoadedModel = useRef(false);

    useEffect(() => {
        // 1. Only register the listener if it hasn't been set up yet
        if (!isListenerRegistered) {
            isListenerRegistered = true;

            listen("engine-output", (event) => {
                let rawText = event.payload;

                // Ignore initialization messages or setup tracing strings
                if (rawText.includes("GGUF metadata") || rawText.includes("model_loaded")) {
                    return;
                }

                if (rawText.startsWith("ai> ")) {
                    rawText = rawText.replace("ai> ", "");
                }

                // 💡 Use functional state update to guarantee we get the absolute latest state
                setMessages((prev) => {
                    // Extra guard: Don't append if the last message is identical (prevents edge case duplicates)
                    if (prev.length > 0 && prev[prev.length - 1].content === rawText && prev[prev.length - 1].role === "assistant") {
                        return prev;
                    }
                    return [...prev, { role: "assistant", content: rawText }];
                });
            });
        }

        // 2. Trigger the backend model startup exactly once
        if (!hasLoadedModel.current) {
            hasLoadedModel.current = true;
            invoke("load_model", { path: "C:\\Projects\\AI_Shell-main\\Bob-llama-Q4_K_S.gguf" })
                .then(res => console.log("Model initialized:", res))
                .catch(err => console.error("Model boot error:", err));
        }

        // Explicitly do NOT return a cleanup unlisten handler here.
        // The listener lives at the application layer safely for the session.
    }, []);

    return (
        <div className="app">
            <Sidebar setSessionId={setSessionId} />
            <ChatWindow sessionId={sessionId} messages={messages} setMessages={setMessages} />
        </div>
    );
}
