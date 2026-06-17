import { useState, useEffect } from "react";
import { invoke } from "@tauri-apps/api/core";
import Sidebar from "./components/Sidebar";
import ChatWindow from "./components/ChatWindow";
import "./styles/App.css";

export default function App() {
  const [sessionId, setSessionId] = useState(Date.now().toString());
  const [messages, setMessages] = useState([]);

  // ⭐ Load GGUF model ONCE when the app starts
 useEffect(() => {
  invoke("load_model", {
    path: "C:\\Projects\\AI_Shell-main\\Bob-llama-Q4_K_S.gguf"
  })
  .then(res => console.log("Engine says:", res))
  .catch(err => console.error("Model load failed:", err));
}, []);


  return (
    <div className="app">
      <Sidebar setSessionId={setSessionId} />
      <ChatWindow
        sessionId={sessionId}
        messages={messages}
        setMessages={setMessages}
      />
    </div>
  );
}
