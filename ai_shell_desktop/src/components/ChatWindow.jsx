import { invoke } from "@tauri-apps/api/core";
import ChatInput from "./ChatInput";
import MessageBubble from "./MessageBubble";

export default function ChatWindow({ sessionId, messages, setMessages }) {
    async function sendMessage(text) {
        if (!text.trim()) return;
        setMessages((prev) => [...prev, { role: "user", content: text }]);
        await invoke("chat", { prompt: text }); // Let engine-output catch the reply
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
