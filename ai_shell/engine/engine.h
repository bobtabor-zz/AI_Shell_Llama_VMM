#pragma once
#include <stddef.h>
#include <stdbool.h>
#include "llama.h"
#include "../vmm/vmm.h"

#ifdef __cplusplus
extern "C" {
#endif
#define MAX_TURNS 32
#define MAX_PATH 260


    // engine.h — canonical tool_call_t and helper (single definition)
#ifndef ENGINE_TOOL_CALL_T
#define ENGINE_TOOL_CALL_T

#include <stdlib.h> // ensure free() is declared

    typedef struct {
        char* name;        // tool name, e.g. "web_search"
        char* arguments;   // JSON string or raw argument text (malloc'd, caller frees)
    } tool_call_t;

    // Single helper only. Use this everywhere to free tool_call_t fields.
    static inline void tool_call_free(tool_call_t* c) {
        if (!c) return;
        if (c->name) { free(c->name); c->name = NULL; }
        if (c->arguments) { free(c->arguments); c->arguments = NULL; }
    }

#endif // ENGINE_TOOL_CALL_T



    extern bool was_chat_console;

    typedef struct {
        char role[8];   // "User" / "AI"
        char text[4096];
    } engine_turn_t;

    typedef struct {
        char role[8];   // "User" / "AI"
        char text[4096];
    } html_turn_t;

    struct llama_context_params ctx_params;


    typedef struct engine {
        struct llama_model* model;
        struct llama_context* ctx;
        //int64_t pos;
        //char conversation[16384];
        struct llama_context_params ctx_params;
		struct llama_model_params model_params;

        html_turn_t html_turns[128];
        int html_n_turns;

        int html_seq_id;   // always 0 for HTML chat

        engine_turn_t turns[MAX_TURNS];
        int n_turns;

        int32_t seq_id;   // single stable sequence
        llama_pos pos;    // current position in this sequence
        // --- Sampling parameters (needed for incremental generation) ---
        float temp;
        int   top_k;
        float top_p;

        vmm_model_t* vmm_model;
        int               cached_vocab_size;
        llama_token_data* candidates_buf;
        struct llama_sampler* sampler;
        // 🌟 ADD THIS: One big persistent batch buffer
        struct llama_batch persistent_batch;

        // ⭐ KV subsystem
        llama_token assistant_tok;
        llama_token inst_end_tok;
        llama_token sys_end_tok;
        int n_past;

        bool    kv_valid;      // is there a meaningful sequence in KV?
        int64_t kv_len;        // how many tokens are currently in KV?

    } engine_t;

    int engine_recreate_context(engine_t* e);

    // Inside engine.h
    void engine_reset(engine_t* e);

   int engine_chat(engine_t* e,
        const char* user_input,
        char* out,
        size_t out_size);

   int engine_chat_html(engine_t* e,
       const char* user_input,
       char* out,
       size_t out_size);

    /*void engine_kv_cache_clear(engine_t* e);*/

    void cmd_open(int argc, char** argv);
    void cmd_close(int argc, char** argv);
    void cmd_infer(int argc, char** argv);

    engine_t* engine_open(const char* path);
    void      engine_close(engine_t* e);

    int engine_generate(
        engine_t* e,
        const char* prompt,
        char* out,
        size_t out_size,
        int max_tokens,
        float temp,
        int top_k,
        float top_p,
        bool stream
    );

        // -------------------------
        // Llama‑3 / Llama‑3.1 / Llama‑3.2  (ChatML)
        // -------------------------

        static const char* LLAMA3_SYSTEM_PROMPT =
            "You are an assistant that can call a tool named \"websearch\".\n"
            "When the user asks for information you cannot know internally, you MUST output ONLY this JSON:\n"
            "{\"tool\":\"websearch\",\"query\":\"search_query\"}\n"
            "You must replace the placeholder with an actual search query.\n"
            "Brief text before it.\n"
            "Brief text after it.\n"
            "Brief explanations.\n"
            "Brief commentary.\n"
            "No blank lines.\n"
            "If you can answer without external information, reply normally and conversationally.";

        /*static const char* LLAMA3_SYSTEM_PROMPT =
            "You are an assistant that can call a tool named \"websearch\".\n"
            "When the user asks for information you cannot know internally, you MUST output ONLY this JSON:\n"
            "{\"tool\":\"websearch\",\"query\":\"search_query\"}\n"
            "You must replace the placeholder with an actual search query.\n"
            "No text before it.\n"
            "No text after it.\n"
            "No explanations.\n"
            "No commentary.\n"
            "No blank lines.\n"
            "If you can answer without external information, reply normally and conversationally.";*/

        // -------------------------
        // Phi‑3 Instruct (ChatML-like)
        // -------------------------
        static const char* PHI3_SYSTEM_PROMPT =
            "You are a helpful assistant. You have access to a websearch tool.\n"
            "If a query requires external info, you MUST call the tool by returning a JSON object.\n"
            "[CRITICAL RULES]\n"
            "1. For tool calls, output ONLY the raw JSON. No markdown, no code fences, no explanations.\n"
            "2. If you know the answer, reply normally in plain text.\n\n"
            "[JSON SCHEMA]\n"
            "{\"tool\": \"websearch\", \"query\": \"optimized search keywords\"}\n\n"
            "[EXAMPLE]\n"
            "User: Who won the latest Super Bowl?\n"
            "Assistant: {\"tool\": \"websearch\", \"query\": \"latest Super Bowl winner\"}";

        // -------------------------
        // SmolLM‑Instruct (SmolLM 1.7B / 2B)
        // MUST be short — model is tiny
        // -------------------------
        static const char* SMOLLM_SYSTEM_PROMPT =
            "You are an AI with a websearch tool.\n"
            "If you need external info, reply ONLY with this JSON:\n"
            "{\"tool\": \"websearch\", \"query\": \"search keywords\"}\n"
            "Rules:\n"
            "- Never use markdown code fences.\n"
            "- Never add extra text or explanations.\n"
            "- If you can answer without the tool, reply with normal text.";

        // ======================================================
        // Mistral‑Instruct / Mixtral‑Instruct — tool‑capable
        // ======================================================
        static const char* MISTRAL_SYSTEM_PROMPT =
            "You are a helpful AI assistant. "
            "When external information is needed, respond ONLY with JSON: "
            "{\"tool\":\"websearch\",\"query\":\"search_query\"}.";

        // ======================================================
        // Qwen‑Instruct (Qwen 1.5 / Qwen 2) — ChatML, tool‑friendly
        // ======================================================
        static const char* QWEN_SYSTEM_PROMPT =
            "You are Qwen, a helpful and knowledgeable assistant. "
            "When external information is required, respond ONLY with a JSON object: "
            "{\"tool\":\"websearch\",\"query\":\"search_query\"}.";

        // ======================================================
        // Gemma‑Instruct (Gemma 1 / Gemma 2) — NOT tool‑trained
        // Keep it short or it collapses.
        // ======================================================
        static const char* GEMMA_SYSTEM_PROMPT =
            "You are a helpful assistant. "
            "When you need external information, output JSON: "
            "{\"tool\":\"websearch\",\"query\":\"search_query\"}.";       

        // ======================================================
        // Llama‑2 Chat — no native tool training
        // Must be simple or it refuses.
        // ======================================================
        static const char* LLAMA2_SYSTEM_PROMPT =
            "You are a helpful, respectful, and honest assistant. "
            "When you need external information, output JSON: "
            "{\"tool\":\"websearch\",\"query\":\"search_query\"}.";



#ifdef __cplusplus
}
#endif
