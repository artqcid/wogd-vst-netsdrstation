#!/usr/bin/env python3
"""Generate C++ bridge parsers from schema/bridge.schema.json.

The JSON Schema (schema/bridge.schema.json) is the single source of truth
for the UI <-> C++ bridge protocol. This script converts it into a header
(source/vst/common/generated/bridge_schema.h) containing:

  - struct BridgeSetParameter { std::string id; double value; }
  - struct BridgeSetStation  { std::string hostPort; }
  - parse functions built on nlohmann::json that validate the message
    against the schema (type, const "type", tuple shape, ParamId enum).

Invoked by CMake as a custom command; the output is committed so builds
work without a Python toolchain (regenerate only when the schema changes):

  python schema/generate-cpp.py schema/bridge.schema.json \\
      --output source/vst/common/generated/bridge_schema.h
"""

import argparse
import json
import re
import sys
from pathlib import Path


def snake_to_pascal(name: str) -> str:
    """BridgeSetParameter from SetParameterMessage."""
    return re.sub(r"(?:^|_)([a-z])", lambda m: m.group(1).upper(), name)


def cpp_identifier(name: str) -> str:
    """Sanitise a schema name into a C++ identifier."""
    return re.sub(r"[^A-Za-z0-9_]", "_", name)


def param_id_list(defs: dict) -> list[str]:
    """The ParamId enum values from the schema definitions."""
    pid = defs.get("ParamId", {})
    return pid.get("enum", [])


def gen_param_ids(ids: list[str]) -> str:
    lines = ['    static const char* const kIds[] = {']
    for i in ids:
        lines.append(f'        "{i}",')
    lines.append('    };')
    lines.append('    for (const char* k : kIds) {')
    lines.append('        if (id == k) return true;')
    lines.append('    }')
    lines.append('    return false;')
    return '\n'.join(lines)


def gen_struct(name: str, fields: list[tuple[str, str, str]]) -> str:
    """Generate a struct with the given (cpp_type, member, default) fields."""
    lines = [f'struct {name} {{']
    for cpp_type, member, default in fields:
        lines.append(f'    {cpp_type} {member}{default};')
    lines.append('};')
    return '\n'.join(lines)


def gen_parse_set_parameter(defs: dict) -> str:
    return f'''inline bool parseSetParameter(const nlohmann::json& j, BridgeSetParameter& out) {{
    if (!j.is_object()) return false;
    if (!j.contains("type") || !j["type"].is_string() || j["type"] != "setParameter") return false;
    if (!j.contains("data") || !j["data"].is_array()) return false;
    const auto& d = j["data"];
    if (d.size() != 2) return false;
    if (!d[0].is_string() || !isParamId(d[0].get<std::string>())) return false;
    if (!d[1].is_number()) return false;
    out.id = d[0].get<std::string>();
    out.value = d[1].get<double>();
    return true;
}}'''


def gen_parse_set_station(defs: dict) -> str:
    return f'''inline bool parseSetStation(const nlohmann::json& j, BridgeSetStation& out) {{
    if (!j.is_object()) return false;
    if (!j.contains("type") || !j["type"].is_string() || j["type"] != "setStation") return false;
    if (!j.contains("data") || !j["data"].is_array()) return false;
    const auto& d = j["data"];
    if (d.size() != 1) return false;
    if (!d[0].is_string() || d[0].get<std::string>().empty()) return false;
    out.hostPort = d[0].get<std::string>();
    return true;
}}'''


def gen_parse_disconnect(defs: dict) -> str:
    return f'''inline bool parseDisconnect(const nlohmann::json& j) {{
    if (!j.is_object()) return false;
    if (!j.contains("type") || !j["type"].is_string() || j["type"] != "disconnect") return false;
    return !j.contains("data") || j["data"].is_null();
}}'''


def gen_parse_get_parameters(defs: dict) -> str:
    return f'''inline bool parseGetParameters(const nlohmann::json& j) {{
    if (!j.is_object()) return false;
    if (!j.contains("type") || !j["type"].is_string() || j["type"] != "getParameters") return false;
    return !j.contains("data") || j["data"].is_null();
}}'''


def build_header(schema: dict) -> str:
    defs = schema.get("definitions", {})
    ids = param_id_list(defs)
    if not ids:
        sys.exit("error: ParamId enum missing from schema definitions")

    parts = []
    parts.append('// AUTO-GENERATED from schema/bridge.schema.json - DO NOT EDIT.')
    parts.append('//')
    parts.append('// Wire contract of the UI <-> C++ bridge (see')
    parts.append('// source/vst/common/bridge_protocol.h for the message')
    parts.append('// envelope). Regenerate with:')
    parts.append('//   python schema/generate-cpp.py schema/bridge.schema.json \\')
    parts.append('//       --output source/vst/common/generated/bridge_schema.h')
    parts.append('#pragma once')
    parts.append('#include <nlohmann/json.hpp>')
    parts.append('#include <string>')
    parts.append('')
    parts.append('namespace netsdr::schema {')
    parts.append('')
    parts.append('// Stable UI-facing parameter names (ParamId in the schema).')
    parts.append('inline bool isParamId(const std::string& id) {')
    parts.append(gen_param_ids(ids))
    parts.append('}')
    parts.append('')
    parts.append('// Result of parsing a setParameter bridge message.')
    parts.append(gen_struct('BridgeSetParameter', [
        ('std::string', 'id', ''),
        ('double', 'value', ' = 0.0'),
    ]))
    parts.append('')
    parts.append('// Result of parsing a setStation bridge message.')
    parts.append(gen_struct('BridgeSetStation', [
        ('std::string', 'hostPort', ''),
    ]))
    parts.append('')
    parts.append(gen_parse_set_parameter(defs))
    parts.append('')
    parts.append(gen_parse_set_station(defs))
    parts.append('')
    parts.append(gen_parse_disconnect(defs))
    parts.append('')
    parts.append(gen_parse_get_parameters(defs))
    parts.append('')
    parts.append('} // namespace netsdr::schema')
    parts.append('')
    return '\n'.join(parts)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('schema', type=Path, help='path to bridge.schema.json')
    parser.add_argument('--output', '-o', type=Path, required=True,
                        help='output header path')
    args = parser.parse_args()

    try:
        schema = json.loads(args.schema.read_text(encoding='utf-8'))
    except FileNotFoundError:
        sys.exit(f'error: schema file not found: {args.schema}')
    except json.JSONDecodeError as e:
        sys.exit(f'error: invalid JSON in {args.schema}: {e}')

    header = build_header(schema)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(header, encoding='utf-8')
    print(f'generated {args.output} ({len(header.splitlines())} lines)')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())