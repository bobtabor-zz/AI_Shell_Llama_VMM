import { useState } from "react";

export default function ChatInput({ onSend }) {
    const [text, setText] = useState("");

    function handleSend() {
        if (!text.trim()) return;
        onSend(text);
        setText("");
    }

    function handleKeyDown(e) {
        if (e.key === "Enter" && !e.shiftKey) {
            e.preventDefault();
            handleSend();
        }
    }

    return (
        <div className="input-bar">
            <textarea
                value={text}
                onChange={(e) => setText(e.target.value)}
                onKeyDown={handleKeyDown}
                placeholder="Type a message..."
            />

            <button onClick={handleSend}>Send</button>
        </div>
    );
}

