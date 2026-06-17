import { invoke } from "@tauri-apps/api/core";

import ChatInput from "./ChatInput";
import MessageBubble from "./MessageBubble";

export default function ChatWindow({ sessionId, messages, setMessages }) {
    async function sendMessage(text) {
        const userMsg = { role: "user", content: text };
        setMessages([...messages, userMsg]);

        const response = await invoke("chat", {
            sessionId,
            prompt: text
        });

        const assistantMsg = { role: "assistant", content: response };
        setMessages((prev) => [...prev, assistantMsg]);
    }

    return (
        <div className="chat-window">
            <div className="messages">
                {messages.map((m, i) => (
                    <MessageBubble key={i} role={m.role} content={m.content} />
                ))}
            </div>

            <ChatInput onSend={sendMessage} />
        </div>
    );
}
