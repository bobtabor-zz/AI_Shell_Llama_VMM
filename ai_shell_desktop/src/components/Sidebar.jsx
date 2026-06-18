import { useState } from "react";

// 📂 Map model names directly to your real local system GGUF files
const AVAILABLE_MODELS = {
    "llama-3-8b": "C:\\Projects\\AI_Shell-main\\Meta-Llama-3-8B-Instruct-Q2_K.gguf",
    "phi-3-mini": "C:\\Projects\\AI_Shell-main\\Phi-3-mini-4k-instruct-q4.gguf",
    "smollm-1.7b": "C:\\Projects\\AI_Shell-main\\smollm-1.7b-instruct-q4_k_m.gguf",
};

export default function Sidebar({ setSessionId, activeSessionId, history, onDeleteSession, onModelChange }) {
    // Sort sessions: newest at the top
    const sessions = Object.keys(history).sort((a, b) => b - a);

    // State to track which item has the active colored background indicator
    const [selectedModel, setSelectedModel] = useState("llama-3-8b");

    // 🛡️ Web-standard confirmation guard (Bypasses Tauri ACL permissions entirely)
    const handleModelSelect = (modelKey) => {
        if (selectedModel === modelKey) return;

        // Use built-in browser engine popup handler
        const confirmed = window.confirm(
            `Are you sure you want to switch to ${modelKey}? This will clear the active engine state.`
        );

        // If cancel clicked, stop right here
        if (!confirmed) return;

        setSelectedModel(modelKey);
        const targetPath = AVAILABLE_MODELS[modelKey];
        if (onModelChange) {
            onModelChange(targetPath); // Fires off the load_model Tauri route event loop
        }
    };

    // 🎯 IMPROVED: Instantly reads the text block to rename the sidebar row
    const getSessionLabel = (id, msgs) => {
        if (!msgs || msgs.length === 0) {
            return `Chat ${new Date(Number(id)).toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}`;
        }

        // Find the very first message sent by the user in this log
        const firstUserMsg = msgs.find(m => m.role === "user");
        if (firstUserMsg && firstUserMsg.content.trim()) {
            const cleanText = firstUserMsg.content.trim();
            return cleanText.length > 20
                ? cleanText.substring(0, 20) + "..."
                : cleanText;
        }

        // Fallback if the engine replied first or text is formatting
        if (msgs && msgs.content) {
            const fallbackText = msgs.content.trim();
            return fallbackText.length > 20 ? fallbackText.substring(0, 20) + "..." : fallbackText;
        }

        return "Empty Chat";
    };

    return (
        <div className="sidebar">
            <button className="new-chat" onClick={() => setSessionId(Date.now().toString())}>
                + New Chat
            </button>

            <h3>Recent History</h3>
            <ul className="list history-list">
                {sessions.map((id) => (
                    <li
                        key={id}
                        className={`history-item ${id === activeSessionId ? "active" : ""}`}
                        onClick={() => setSessionId(id)}
                    >
                        <span className="history-text">💬 {getSessionLabel(id, history[id])}</span>

                        <button
                            className="delete-btn"
                            title="Delete Chat"
                            onClick={(e) => {
                                e.stopPropagation();
                                onDeleteSession(id);
                            }}
                        >
                            ❌
                        </button>
                    </li>
                ))}
            </ul>

            <h3>Models</h3>
            <ul className="list model-list">
                {Object.keys(AVAILABLE_MODELS).map((modelKey) => (
                    <li
                        key={modelKey}
                        className={`model-item ${selectedModel === modelKey ? "selected" : ""}`}
                        onClick={() => handleModelSelect(modelKey)}
                    >
                        🤖 {modelKey}
                    </li>
                ))}
            </ul>

            <h3>Plugins</h3>
            <div className="list">
                <label><input type="checkbox" /> websearch</label>
                <label><input type="checkbox" /> summarize</label>
            </div>
        </div>
    );
}
