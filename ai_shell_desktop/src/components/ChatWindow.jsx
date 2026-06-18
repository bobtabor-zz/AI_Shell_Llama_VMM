import { useEffect, useRef } from "react"; // ⭐ Added hooks
import { invoke } from "@tauri-apps/api/core";
import ChatInput from "./ChatInput";
import MessageBubble from "./MessageBubble";

export default function ChatWindow({ sessionId, messages, setMessages }) {
    // ⚓ Create an invisible anchor at the bottom of the messages list
    const messagesEndRef = useRef(null);

    const scrollToBottom = () => {
        // scrollIntoView moves the window view to this target div smoothly
        messagesEndRef.current?.scrollIntoView({ behavior: "smooth" });
    };

    // 🔥 Listen for changes to the messages array and instantly push down
    useEffect(() => {
        scrollToBottom();
    }, [messages]);

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
                {/* ⭐ The empty element acting as our layout anchor */}
                <div ref={messagesEndRef} />
            </div>
            <ChatInput onSend={sendMessage} />
        </div>
    );
}
