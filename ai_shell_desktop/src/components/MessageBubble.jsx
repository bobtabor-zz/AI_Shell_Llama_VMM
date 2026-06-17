export default function MessageBubble({ role, content }) {
    return (
        <div className={`bubble ${role}`}>
            <div className="bubble-inner">
                {content}
            </div>
        </div>
    );
}
