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

    static const char* TOOL_SYSTEM_PROMPT =
        "You are a helpful assistant. If a query requires external info, you MUST call the websearch tool.\n"
        "To call it, output ONLY this raw JSON object, no code fences, no extra text:\n"
        "{\"tool\":\"websearch\",\"query\":\"<user query>\"}\n"
        "If you can answer using internal knowledge, reply normally without JSON.";


   /* static const char* TOOL_SYSTEM_PROMPT =
        "You are a helpful assistant.\n"
        "\n"
        "You have access to a tool named \"websearch\".\n"
        "\n"
        "When the user asks a question that requires external information, you MUST respond with a JSON object in the following exact format:\n"
        "\n"
        "{\"tool\":\"websearch\",\"query\":\"<the user question here>\"}\n"
        "\n"
        "Rules:\n"
        "- Output ONLY the JSON object.\n"
        "- Do NOT add explanations, commentary, or text before or after the JSON.\n"
        "- Do NOT wrap the JSON in code fences.\n"
        "- Do NOT change the field names \"tool\" or \"query\".\n"
        "- Do NOT add extra fields.\n"
        "- Do NOT answer the question yourself until AFTER the tool result is provided.\n"
        "- If you add anything outside the JSON object, the tool call will fail.\n"
        "\n"
        "After the tool result is provided, you may answer normally\n"
        "\n"
        "If the user asks something that you can answer using your internal knowledge, answer normally without calling the tool.\n";*/

   /* static const char* websearch_grammar =
        "root ::= object\n"
        "object ::= '{\"tool\":\"websearch\",\"query\":\"' query '\"}'\n"
        "query ::= char*\n"
        "char ::= [^\"\\\\] | escape\n"
        "escape ::= '\\\\' [\"\\\\/bfnrt]\n";*/
        // Paste this exactly as-is. The backslashes ensure that both C and GBNF 
        // 
    // read the internal double quotes perfectly.
    static const char* websearch_grammar =
        "root   ::= object\n"
        "object ::= \"{\\\"tool\\\":\\\"websearch\\\",\\\"query\\\":\\\"\" query \"\\\"}\"\n"
        "query  ::= [^\\\"]*\n";




#ifdef __cplusplus
}
#endif
