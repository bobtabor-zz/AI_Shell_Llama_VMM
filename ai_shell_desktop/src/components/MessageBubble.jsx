import React from "react";
import ReactMarkdown from "react-markdown";

export default function MessageBubble({ role, content }) {
    // Helper function to process raw server JSON payload data strings
    const parseContent = (rawContent) => {
        if (typeof rawContent !== "string") return rawContent;

        const trimmed = rawContent.trim();

        // Check if the incoming message is a raw server JSON payload object string
        if (trimmed.startsWith("{") && trimmed.endsWith("}")) {
            try {
                const parsed = JSON.parse(trimmed);
                let markdownString = "";

                if (parsed.query) markdownString += `### 🔍 Search: "${parsed.query}"\n\n`;
                if (parsed.message) markdownString += `*${parsed.message}*\n\n`;

                if (parsed.results && Array.isArray(parsed.results)) {
                    parsed.results.forEach((item, index) => {
                        const associatedImg = parsed.images && parsed.images[index];

                        // FIX: We pass a single image syntax block, but clean up inner quotes
                        if (associatedImg) {
                            const safeTitle = (item.title || "")
                                .replace(/"/g, "'")
                                .replace(/[\n\r]+/g, " ");

                            // We render a standard custom markdown image element block
                            markdownString += `![${item.source || "Image"}](${associatedImg} "${safeTitle}")\n\n`;
                        } else if (item.title) {
                            // Fallback text rendering ONLY if there is no image block for this index slot
                            markdownString += `**Source (${item.source || "Web"}):** ${item.title}\n\n`;
                            if (item.url) {
                                markdownString += `[View Web Link](${item.url})\n\n`;
                            }
                            markdownString += `---\n\n`;
                        }
                    });
                }
                return markdownString;
            } catch (e) {
                return rawContent;
            }
        }

        return rawContent;
    };

    const displayMarkdown = parseContent(content);

    return (
        <div className={`bubble ${role}`} style={styles.bubbleContainer}>
            <ReactMarkdown
                components={{
                    // Custom interceptor logic for standard hyperlinks
                    a: ({ node, ...props }) => (
                        <span style={styles.urlContainer}>
                            <span style={styles.urlLabel}>Link:</span>
                            <a
                                {...props}
                                target="_blank"
                                rel="noopener noreferrer"
                                style={styles.highlightedUrl}
                            />
                        </span>
                    ),
                    // Custom card container logic for rendering visual blocks vertically
                    img: ({ node, ...props }) => (
                        <div style={styles.card}>
                            <a href={props.src} target="_blank" rel="noopener noreferrer" style={styles.imgLink}>
                                <img
                                    {...props}
                                    style={styles.cardImg}
                                    alt={props.alt || "Search item image"}
                                />
                            </a>
                            {props.title && (
                                <div style={styles.cardTitle}>
                                    <strong>Source ({props.alt || "Web"}):</strong> {props.title}
                                </div>
                            )}
                        </div>
                    ),
                    code: ({ node, inline, ...props }) =>
                        inline ? (
                            <code {...props} style={{ background: "#eee", padding: "2px 4px" }} />
                        ) : (
                            <pre
                                style={{
                                    background: "#1e1e1e",
                                    color: "#fff",
                                    padding: "12px",
                                    borderRadius: "8px",
                                    overflowX: "auto",
                                }}
                            >
                                <code {...props} />
                            </pre>
                        ),
                }}
            >
                {displayMarkdown}
            </ReactMarkdown>
        </div>
    );
}

// Scannable, modern block layouts that force downward vertical stacking
const styles = {
    bubbleContainer: {
        display: 'block',
        width: '100%',
        clear: 'both',
        boxSizing: 'border-box'
    },
    card: {
        backgroundColor: '#ffffff',
        border: '1px solid #e1e8ed',
        borderRadius: '10px',
        overflow: 'hidden',
        display: 'block',            // Forces each card container onto its own line
        width: '100%',
        maxWidth: '480px',           // Keeps the card compact on larger screens
        margin: '16px 0',            // Provides clear spacing between stacked items
        boxShadow: '0 2px 8px rgba(0,0,0,0.06)',
        boxSizing: 'border-box'
    },
    imgLink: {
        display: 'block',
        width: '100%',
        backgroundColor: '#f8f9fa'
    },
    cardImg: {
        width: '100%',               // Forces scaling down to fit the container bounds
        maxWidth: '100%',            // Prevents spilling off the edge of mobile screens
        height: 'auto',              // Preserves the image's original aspect ratio
        maxHeight: '280px',          // Restricts overly tall vertical images
        objectFit: 'contain',        // Ensures the whole photo is visible without cropping
        display: 'block'
    },
    cardTitle: {
        padding: '12px 16px',
        fontSize: '0.88rem',
        color: '#2c3e50',
        borderTop: '1px solid #e1e8ed',
        backgroundColor: '#f8f9fa',
        lineHeight: '1.5',
        overflow: 'visible',         // Allows descriptions to wrap fully
        whiteSpace: 'normal',        // Prevents truncation dots from cutting text off
        wordBreak: 'break-word'
    },
    urlContainer: {
        display: 'block',            // Forces links onto their own rows
        backgroundColor: '#fdf6e2',
        border: '1px solid #f5e6c4',
        padding: '8px 12px',
        borderRadius: '6px',
        margin: '8px 0',
        maxWidth: '480px',
        wordBreak: 'break-all'
    },
    urlLabel: {
        fontSize: '0.65rem',
        fontWeight: 'bold',
        color: '#b58900',
        marginRight: '6px',
        textTransform: 'uppercase'
    },
    highlightedUrl: {
        color: '#dd4b39',
        fontSize: '0.85rem',
        fontWeight: '600',
        textDecoration: "underline"
    }
};
