export default function Sidebar({ setSessionId }) {
    return (
        <div className="sidebar">
            <button
                className="new-chat"
                onClick={() => setSessionId(Date.now().toString())}
            >
                + New Chat
            </button>

            <h3>Models</h3>
            <ul className="list">
                <li>llama-3-8b</li>
                <li>phi-3-mini</li>
                <li>smollm-1.7b</li>
            </ul>

            <h3>Plugins</h3>
            <div className="list">
                <label><input type="checkbox" /> websearch</label>
                <label><input type="checkbox" /> summarize</label>
            </div>
        </div>
    );
}
