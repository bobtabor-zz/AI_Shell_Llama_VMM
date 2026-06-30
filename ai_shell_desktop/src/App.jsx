import { useState, useEffect, useRef } from "react";
import { invoke } from "@tauri-apps/api/core";
import { listen } from "@tauri-apps/api/event";
import Sidebar from "./components/Sidebar";
import ChatWindow from "./components/ChatWindow";
import "./styles/App.css";

let isListenerRegistered = false;

export default function App() {
    // 💾 Initializer function reads initial session history or starts fresh 
    const [history, setHistory] = useState(() => {
        const saved = localStorage.getItem("chat_history_sessions");
        return saved ? JSON.parse(saved) : {};
    });

    const [sessionId, setSessionId] = useState(() => Date.now().toString());

    // 🎯 CRITICAL FIX: Keep a live mutable reference to the active session ID 
    const activeSessionIdRef = useRef(sessionId);

    // Keep the reference synchronized whenever the state updates 
    useEffect(() => {
        activeSessionIdRef.current = sessionId;
    }, [sessionId]);

    const messages = history[sessionId] || [];
    const hasLoadedModel = useRef(false);

    // 📝 Helper to update messages inside a specific session ID safely 
    const updateSessionMessages = (id, updaterFn) => {
        setHistory((prevHistory) => {
            const currentSessionMessages = prevHistory[id] || [];
            const updatedMessages = typeof updaterFn === "function" ? updaterFn(currentSessionMessages) : updaterFn;
            const newHistory = {
                ...prevHistory,
                [id]: updatedMessages,
            };
            localStorage.setItem("chat_history_sessions", JSON.stringify(newHistory));
            return newHistory;
        });
    };

    // 🗑️ Function to delete a chat session from state and LocalStorage 
    const deleteSession = (idToDelete) => {
        setHistory((prevHistory) => {
            const newHistory = { ...prevHistory };
            delete newHistory[idToDelete];
            localStorage.setItem("chat_history_sessions", JSON.stringify(newHistory));
            return newHistory;
        });

        if (sessionId === idToDelete) {
            setSessionId(Date.now().toString());
        }
    };

    // 🌟 NEW INTERCEPTION PLUMBING: Wires the sidebar select handler to your Rust backend 
    const changeModelPath = async (newPath) => {
        // ⚙️ Print a helpful loading text directly inside your visible frontend bubble space 
        updateSessionMessages(sessionId, (prev) => [
            ...prev,
            { role: "assistant", content: "🔄 System: UNLOAD triggered. Swapping models..." }
        ]);

        try {
            // 🚀 This makes the direct call to your updated load_model command inside main.rs! 
            const result = await invoke("load_model", { path: newPath });
            console.log("Backend Model Swap Status:", result);
        } catch (err) {
            console.error("Failed to sequence model swap:", err);
            updateSessionMessages(sessionId, (prev) => [
                ...prev,
                { role: "assistant", content: `❌ Error routing swap: ${err}` }
            ]);
        }
    };

    // 🌍 Global Event Listener 
    useEffect(() => {
        if (!isListenerRegistered) {
            isListenerRegistered = true;
            listen("engine-output", (event) => {
                let rawText = event.payload;
                if (rawText.includes("GGUF metadata") || rawText.includes("model_loaded")) return;
                if (rawText.startsWith("ai> ")) rawText = rawText.replace("ai> ", "");

                const currentId = activeSessionIdRef.current;

                updateSessionMessages(currentId, (prev) => {
                    if (prev.length > 0 && prev[prev.length - 1].content === rawText && prev[prev.length - 1].role === "assistant") {
                        return prev;
                    }
                    return [...prev, { role: "assistant", content: rawText }];
                });
            });
        }

        if (!hasLoadedModel.current) {
            hasLoadedModel.current = true;
            // Initial startup model loading execution sequence 
            invoke("load_model", { path: "C:\\Projects\\AI_Shell-main\\Hermes-2-Pro-Llama-3-8B-Q4_K_M.gguf" })
                .then(res => console.log("Model initialized:", res))
                .catch(err => console.error("Model boot error:", err));
        }
    }, []);

    const setMessages = (updater) => {
        updateSessionMessages(sessionId, updater);
    };

    return (
        <div className="app">
            <Sidebar
                setSessionId={setSessionId}
                activeSessionId={sessionId}
                history={history}
                onDeleteSession={deleteSession}
                onModelChange={changeModelPath}
            />
            <ChatWindow
                sessionId={sessionId}
                messages={messages}
                setMessages={setMessages}
            />
        </div>
    );
}
