## AI commit attribution

- Every commit created by an AI assistant must include an `Assisted-by` Git trailer.
- Prefer the specific model name and reasoning effort when confirmed by the
  session metadata or explicitly by the user, rather than only the application
  name (such as Codex).
- Example: `Assisted-by: GPT-6 Astra Medium`. This is the user's stated model
  and reasoning level for the session that added this rule, not a default for
  future agents. Use your own confirmed identity and effort.
- If the model or reasoning effort is unavailable, omit the unknown detail;
  use the assistant's name if the model is unknown. Never guess.
- Preserve existing attribution trailers when amending commits, and add your
  own if it is not already present. Do not rewrite pushed history merely to
  change attribution unless the user explicitly requests that rewrite.
