import React from "react";
import ReactMarkdown from "react-markdown";

export default function MessageBubble({ role, content }) {
    // Helper function to safely detect and process search payload JSON strings
    const parseContent = (rawContent) => {
        if (typeof rawContent !== "string") return rawContent;

        const trimmed = rawContent.trim();
        // Look for typical JSON brackets
        if (trimmed.startsWith("{") && trimmed.endsWith("}")) {
            try {
                const parsed = JSON.parse(trimmed);

                // Construct a structured Markdown layout out of the raw response payload
                let markdownString = `### 🔍 Results for "${parsed.query || 'Search'}"\n\n`;
                markdownString += `${parsed.message || ''}\n\n`;

                if (parsed.results && Array.isArray(parsed.results)) {
                    parsed.results.forEach((item) => {
                        // Filter out non-image aggregate pages like pexels search directories
                        const isDirectImage = /\.(jpg|jpeg|png|gif|webp)/i.test(item.url) || item.url?.includes("staticflickr");

                        if (isDirectImage) {
                            // Format image template: ![alt text](url "Title text component")
                            markdownString += `![${item.title || 'Cat Picture'}](${item.url} "${item.title || ''}")\n\n`;
                        } else if (item.url) {
                            // Fallback link format for standard external search pages
                            markdownString += `* **Source:** [${item.title || item.url}](${item.url})\n\n`;
                        }
                    });
                }
                return markdownString;
            } catch (e) {
                // If parsing fails, fall back to rendering the raw text string gracefully
                return rawContent;
            }
        }
        return rawContent;
    };

    const displayMarkdown = parseContent(content);

    return (
        <div className={`bubble ${role}`}>
            <ReactMarkdown
                components={{
                    // Intercept and style link components
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
                    // Intercept and style image components as card containers
                    img: ({ node, ...props }) => (
                        <div style={styles.card}>
                            <a href={props.src} target="_blank" rel="noopener noreferrer" style={styles.imgLink}>
                                <img
                                    {...props}
                                    style={styles.cardImg}
                                    alt={props.alt || "Search item"}
                                />
                            </a>
                            {props.title && <div style={styles.cardTitle}>{props.title}</div>}
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

// Scannable layouts for the parsed card blocks
const styles = {
    card: {
        backgroundColor: '#ffffff',
        border: '1px solid #e1e8ed',
        borderRadius: '10px',
        overflow: 'hidden',
        display: 'inline-flex',
        flexDirection: 'column',
        width: '100%',
        maxWidth: '280px',
        margin: '10px 8px 10px 0',
        boxShadow: '0 2px 8px rgba(0,0,0,0.06)',
        verticalAlign: 'top'
    },
    imgLink: {
        display: 'block',
        height: '160px',
        backgroundColor: '#000'
    },
    cardImg: {
        width: '100%',
        height: '100%',
        objectFit: 'cover'
    },
    cardTitle: {
        padding: '8px 12px',
        fontSize: '0.8rem',
        color: '#2c3e50',
        borderTop: '1px solid #e1e8ed',
        backgroundColor: '#f8f9fa',
        fontWeight: '500',
        lineHeight: '1.3',
        overflow: 'hidden',
        textOverflow: 'ellipsis',
        whiteSpace: 'nowrap'
    },
    urlContainer: {
        display: 'inline-block',
        backgroundColor: '#fdf6e2',
        border: '1px solid #f5e6c4',
        padding: '2px 6px',
        borderRadius: '4px',
        margin: '2px 4px',
        wordBreak: 'break-all',
        verticalAlign: 'middle'
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
        fontSize: '0.82rem',
        fontWeight: '600',
        textDecoration: "underline"
    }
};
