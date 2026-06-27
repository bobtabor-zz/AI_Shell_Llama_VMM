import React from "react";
import ReactMarkdown from "react-markdown";

export default function MessageBubble({ role, content }) {
    // Helper function to process raw server JSON payload data strings
    const parseContent = (rawContent) => {
        if (typeof rawContent !== "string") return rawContent;

        const trimmed = rawContent.trim();

      
        // Helper: remove all Chromium garbage safely without dropping actual content
        const clean = (text) => {
            if (!text) return "";
            return text
                // 1. Strip raw HTML code blocks completely
                .replace(/<style[\s\S]*?<\/style>/gi, "")
                .replace(/<script[\s\S]*?<\/script>/gi, "")
                .replace(/<svg[\s\S]*?<\/svg>/gi, "")
                .replace(/<\/?[^>]+(>|$)/g, "")

                // 2. TARGET CHROME JUNK: Nuke /style, /script, /svg even when glued to words (e.g., /scriptGoogle -> Google)
                // This keeps safe words like "wiki" and "ddc" perfectly fine because it strictly checks for tag names.
                .replace(/\/?\b(style|script|svg)(?=[A-Z\s]|$)/gi, "")
                .replace(/\/?\b(style|script|svg)\b/gi, "")

                // 3. Sweep any accidental leftover duplicate slashes
                .replace(/\s+\/\s+/g, " ")

                // 4. Compact the spacing back to a clean readable sentence
                .replace(/\s+/g, " ")
                .trim();
        };


        if (trimmed.startsWith("{") && trimmed.endsWith("}")) {
            try {
                const parsed = JSON.parse(trimmed);
                let markdownString = "";

                // Clean top-level fields
                const cleanQuery = clean(parsed.query);
                const cleanMessage = clean(parsed.message);
                const cleanSnippet = clean(parsed.snippet);
                const cleanTitle = clean(parsed.title);

                // Search header
                if (cleanQuery) markdownString += `### 🔍 Search: "${cleanQuery}"\n\n`;
                if (cleanMessage) markdownString += `*${cleanMessage}*\n\n`;
                if (cleanTitle && cleanTitle !== "Chromium HTML snapshot") {
                    markdownString += `## ${cleanTitle}\n\n`;
                }

                // Clean snippet
                if (cleanSnippet) {
                    markdownString += `${cleanSnippet}\n\n`;
                }

                // Top-level URL
                if (parsed.url) {
                    // Encodes spaces as %20 so Markdown can parse it as a real, clickable link
                    markdownString += `[View Web Link](${encodeURI(parsed.url)})\n\n`;
                }


                // ChatGPT-style cards
                if (parsed.results && Array.isArray(parsed.results)) {
                    parsed.results.forEach((item, index) => {
                        const img = parsed.images?.[index] || null;

                        const cleanItemTitle = clean(item.title);
                        const cleanItemSnippet = clean(item.snippet);

                        // Title
                        if (cleanItemTitle) {
                            markdownString += `## ${cleanItemTitle}\n\n`;
                        }

                        // Image
                        if (img) {
                            const safeTitle = cleanItemTitle.replace(/"/g, "'");
                            markdownString += `![Image](${img} "${safeTitle}")\n\n`;
                        }

                        // Snippet
                        if (cleanItemSnippet) {
                            markdownString += `${cleanItemSnippet}\n\n`;
                        }

                        // URL
                        if (item.url) {
                            // Encodes search query spaces inside individual search result items too
                            markdownString += `[View Web Link](${encodeURI(item.url)})\n\n`;
                        }


                        markdownString += `---\n\n`;
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
    urlContainer: {
        display: 'inline-flex',
        alignItems: 'center',
        margin: '6px 0',
        padding: '6px 12px',
        backgroundColor: '#e0f2fe', // Soft modern blue highlight background
        border: '1px solid #bae6fd',
        borderRadius: '6px',
    },
    urlLabel: {
        fontWeight: 'bold',
        color: '#0369a1',
        marginRight: '6px',
        fontSize: '13px',
    },
    highlightedUrl: {
        color: '#0252c5',          // High-visibility deep blue link text
        textDecoration: 'underline',
        fontWeight: '600',
        fontSize: '14px',
    },
    card: {
        backgroundColor: '#ffffff',
        border: '1px solid #e1e8ed',
        borderRadius: '10px',
        overflow: 'hidden',
        display: 'block',
        width: '100%',
        maxWidth: '480px',
        marginTop: '12px',
        marginBottom: '12px'
    },
    cardImg: {
        width: '100%',
        height: 'auto',
        display: 'block'
    },
    imgLink: {
        display: 'block'
    },
    cardTitle: {
        padding: '10px',
        fontSize: '13px',
        color: '#333',
        borderTop: '1px solid #e1e8ed',
        backgroundColor: '#f8f9fa'
    }
};

