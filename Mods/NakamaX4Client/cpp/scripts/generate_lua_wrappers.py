#!/usr/bin/env python3
"""
Lua Wrapper Generator for X4 C++ Scripts

This script scans C++ header files for functions marked with // LUA_EXPORT
and generates Lua wrapper functions automatically.
"""

import re
import argparse
import os
from typing import List, Dict, Tuple

class FunctionSignature:
    def __init__(self, return_type: str, name: str, params: List[Tuple[str, str]], class_name: str = "", header_file: str = "", namespace: str = ""):
        self.return_type = return_type.strip()
        self.name = name.strip()
        self.params = params
        self.class_name = class_name.strip()
        self.header_file = header_file
        self.namespace = namespace.strip()

    def __str__(self):
        param_str = ", ".join(f"{t} {n}" for t, n in self.params)
        return f"{self.return_type} {self.name}({param_str})"

def parse_cpp_function_signature(line: str) -> FunctionSignature:
    """Parse a C++ function signature from a string."""
    # Remove any leading/trailing whitespace
    line = line.strip()
    
    # Remove implementation body if present (anything from { to the end)
    line = re.sub(r'\s*\{.*', '', line)
    
    # Remove any trailing whitespace and semicolons
    line = line.strip().rstrip(';')

    # Remove any macros like NAKAMA_X4_API, and keywords like override, const at end
    line = re.sub(r'\b\w+_API\s+', '', line)
    line = re.sub(r'\s+override\s*$', '', line)
    line = re.sub(r'\s+const\s*$', '', line)

    # Match function signature pattern - handle const pointers and complex types with namespaces
    pattern = r'^((?:const\s+)?(?:\w+::)*\w+(?:\s*\*)*)\s+(\w+)\s*\((.*)\)$'
    match = re.match(pattern, line)

    if not match:
        raise ValueError(f"Invalid function signature: {line}")

    return_type = match.group(1)
    func_name = match.group(2)
    params_str = match.group(3).strip()

    # Parse parameters
    params = []
    if params_str and params_str != "void":
        # Split by comma, but be careful with template types
        param_list = split_params(params_str)
        for param in param_list:
            param = param.strip()
            if param:
                # Parse type and name using regex to handle default values
                param_match = re.match(r'^(.*\S)\s+(\w+)(\s*=.*)?$', param)
                if param_match:
                    param_type = param_match.group(1).strip()
                    param_name = param_match.group(2)
                    params.append((param_type, param_name))
                else:
                    print(f"Warning: Could not parse parameter '{param}'")

    return FunctionSignature(return_type, func_name, params)

def scan_header_files_for_exports(header_dir: str) -> List[FunctionSignature]:
    """Scan all header files for // LUA_EXPORT comments and extract function signatures with class context."""
    functions = []
    
    for root, dirs, files in os.walk(header_dir):
        for file in files:
            if file.endswith('.h') or file.endswith('.hpp'):
                file_path = os.path.join(root, file)
                try:
                    with open(file_path, 'r', encoding='utf-8') as f:
                        content = f.read()
                except UnicodeDecodeError:
                    continue
                
                # Split into lines for processing
                lines = content.split('\n')
                current_class = ""
                current_namespace = ""
                brace_depth = 0
                namespace_depth = 0
                
                i = 0
                while i < len(lines):
                    line = lines[i].strip()
                    
                    # Track namespace definitions
                    if line.startswith('namespace '):
                        # Extract namespace name
                        ns_match = re.match(r'namespace\s+(\w+)', line)
                        if ns_match:
                            current_namespace = ns_match.group(1)
                            # Look ahead to find the opening brace
                            k = i + 1
                            while k < len(lines) and '{' not in lines[k]:
                                k += 1
                            if k < len(lines) and '{' in lines[k]:
                                namespace_depth = lines[k].count('{') - lines[k].count('}')
                            else:
                                namespace_depth = 0
                    
                    # Track class definitions
                    if line.startswith('class '):
                        # Extract class name
                        class_match = re.match(r'class\s+(\w+)', line)
                        if class_match:
                            current_class = class_match.group(1)
                            # Look ahead to find the opening brace
                            k = i + 1
                            while k < len(lines) and '{' not in lines[k]:
                                k += 1
                            if k < len(lines) and '{' in lines[k]:
                                brace_depth = lines[k].count('{') - lines[k].count('}')
                            else:
                                brace_depth = 0
                    
                    # Track brace depth for nested structures
                    brace_depth += line.count('{') - line.count('}')
                    namespace_depth += line.count('{') - line.count('}')
                    
                    # If we exit a class definition, clear current_class
                    if brace_depth <= 0 and current_class:
                        current_class = ""
                    
                    # If we exit a namespace definition, clear current_namespace
                    if namespace_depth <= 0 and current_namespace:
                        current_namespace = ""
                    
                    # Look for LUA_EXPORT comments
                    if '// LUA_EXPORT' in line:
                        # Find the function signature on subsequent lines
                        j = i + 1
                        func_lines = []
                        while j < len(lines):
                            next_line = lines[j].strip()
                            if next_line and not next_line.startswith('//'):
                                func_lines.append(next_line)
                                if ';' in next_line or '{' in next_line:
                                    break
                            j += 1
                        
                        if func_lines:
                            func_signature = ' '.join(func_lines).strip()
                            # Remove implementation if present
                            func_signature = re.sub(r'\s*\{.*\}', '', func_signature).strip()
                            if func_signature.endswith(';'):
                                func_signature = func_signature[:-1].strip()
                            
                            try:
                                parsed = parse_cpp_function_signature(func_signature)
                                parsed.class_name = current_class
                                parsed.namespace = current_namespace
                                parsed.header_file = file_path
                                functions.append(parsed)
                                print(f"Found LUA_EXPORT function: {parsed} in class {current_class}")
                            except ValueError as e:
                                print(f"Warning: Could not parse function signature '{func_signature}': {e}")
                    
                    i += 1
    
    return functions

def split_params(params_str: str) -> List[str]:
    """Split parameter string by commas, respecting templates and parentheses."""
    params = []
    current = ""
    level = 0

    for char in params_str:
        if char == ',' and level == 0:
            params.append(current)
            current = ""
        else:
            current += char
            if char in '<([':
                level += 1
            elif char in '>)':
                level -= 1

    if current:
        params.append(current)

    return params

def generate_lua_wrapper(func: FunctionSignature) -> str:
    """Generate a Lua wrapper function for the given C++ function."""

    # Skip functions with complex parameters we can't handle yet
    for param_type, param_name in func.params:
        if "&" in param_type and not "std::string" in param_type and not "Config" in param_type:
            return f"// Skipped {func.name} - complex parameter type {param_type}\n\n"

    lua_func_name = f"lua_{func.name}"

    # Generate parameter checking and extraction
    param_checks = []
    param_extracts = []
    param_index = 1

    for param_type, param_name in func.params:
        if "const char*" in param_type:
            param_checks.append(f'    const char* {param_name} = luaL_checkstring(L, {param_index});')
            param_extracts.append(f'{param_name}')
            param_index += 1
        elif param_type == "std::string" or "const std::string&" in param_type:
            param_checks.append(f'    std::string {param_name} = luaL_checkstring(L, {param_index});')
            param_extracts.append(f'{param_name}')
            param_index += 1
        elif "int" in param_type or "long" in param_type:
            param_checks.append(f'    {param_type} {param_name} = luaL_checkinteger(L, {param_index});')
            param_extracts.append(f'{param_name}')
            param_index += 1
        elif "bool" in param_type:
            param_checks.append(f'    bool {param_name} = lua_toboolean(L, {param_index});')
            param_extracts.append(f'{param_name}')
            param_index += 1
        elif "float" in param_type or "double" in param_type:
            param_checks.append(f'    {param_type} {param_name} = luaL_checknumber(L, {param_index});')
            param_extracts.append(f'{param_name}')
            param_index += 1
        elif "Config" in param_type:
            # Special handling for Config struct - expect host, port, serverKey, useSSL as separate parameters
            param_checks.append(f'    std::string config_host = luaL_checkstring(L, {param_index});')
            param_checks.append(f'    int config_port = luaL_checkinteger(L, {param_index + 1});')
            param_checks.append(f'    std::string config_serverKey = luaL_checkstring(L, {param_index + 2});')
            param_checks.append(f'    bool config_useSSL = lua_toboolean(L, {param_index + 3});')
            param_checks.append(f'    NakamaX4Client::Config {param_name}_temp = {{config_host, config_port, config_serverKey, config_useSSL}};')
            param_checks.append(f'    const NakamaX4Client::Config& {param_name} = {param_name}_temp;')
            param_extracts.append(f'{param_name}')
            param_index += 4
        else:
            # Generic parameter
            param_checks.append(f'    // TODO: Handle parameter {param_name} of type {param_type}')
            param_extracts.append(f'{param_name}')
            param_index += 1

    # Generate function call
    if func.class_name:
        base_call = f"{func.class_name}::GetInstance()->{func.name}({', '.join(param_extracts)})"
    else:
        if func.namespace:
            base_call = f"{func.namespace}::{func.name}({', '.join(param_extracts)})"
        else:
            base_call = f"{func.name}({', '.join(param_extracts)})"

    # Generate return value handling
    if func.return_type == "void":
        call = f"    {base_call};"
        return_stmt = "    return 0;"
    elif func.return_type in ["int", "long", "long long"]:
        call = f"    auto result = {base_call};"
        return_stmt = f"    lua_pushinteger(L, result);\n    return 1;"
    elif func.return_type == "bool":
        call = f"    auto result = {base_call};"
        return_stmt = f"    lua_pushboolean(L, result);\n    return 1;"
    elif func.return_type == "const char*" or func.return_type == "std::string":
        call = f"    auto result = {base_call};"
        return_stmt = f"    lua_pushstring(L, result.c_str());\n    return 1;"
    elif func.return_type == "AuthResult":
        call = f"    auto result = {base_call};"
        return_stmt = f"    PushAuthResult(L, result);\n    return 1;"
    elif func.return_type == "SyncResult":
        call = f"    auto result = {base_call};"
        return_stmt = f"    PushSyncResult(L, result);\n    return 1;"
    elif func.return_type == "float" or func.return_type == "double":
        call = f"    auto result = {base_call};"
        return_stmt = f"    lua_pushnumber(L, result);\n    return 1;"
    else:
        # Complex return type - assume it's a struct/table
        call = f"    auto result = {base_call};"
        return_stmt = f"    // TODO: Handle return type {func.return_type}\n    return 1;"

    # Generate the complete wrapper function
    wrapper = f"""static int {lua_func_name}(lua_State* L) {{
"""
    if param_checks:
        wrapper += "\n".join(param_checks) + "\n\n"

    wrapper += f"""{call}
{return_stmt}
}}

"""

    return wrapper

def generate_lua_module_registration(functions: List[FunctionSignature], module_name: str) -> str:
    """Generate the Lua module registration code."""
    reg_entries = []
    for func in functions:
        # Skip functions with complex parameters we can't handle
        skip_func = False
        for param_type, param_name in func.params:
            if "&" in param_type and not "std::string" in param_type and not "Config" in param_type:
                skip_func = True
                break
        if not skip_func:
            reg_entries.append(f'    {{"{func.name}", lua_{func.name}}},')

    registration = f"""static const luaL_Reg {module_name}_functions[] = {{
{chr(10).join(reg_entries)}
    {{NULL, NULL}} // Sentinel
}};

#ifdef _WIN32
__declspec(dllexport)
#endif
int luaopen_{module_name}(lua_State* L) {{
    luaL_register(L, "{module_name}", {module_name}_functions);
    return 1;
}}

"""

    return registration

def main():
    parser = argparse.ArgumentParser(description="Generate Lua wrappers for C++ functions marked with // LUA_EXPORT")
    parser.add_argument("header_dir", help="Directory containing .h files to scan for LUA_EXPORT functions")
    parser.add_argument("-o", "--output_dir", help="Output directory for generated Lua wrapper files")

    args = parser.parse_args()

    # Scan header files for LUA_EXPORT marked functions
    functions = scan_header_files_for_exports(args.header_dir)

    if not functions:
        print("No LUA_EXPORT functions found in header files")
        return

    # Group functions by header file
    functions_by_header = {}
    for func in functions:
        header_file = func.header_file
        if header_file not in functions_by_header:
            functions_by_header[header_file] = []
        functions_by_header[header_file].append(func)

    # Generate wrapper files for each header
    output_dir = args.output_dir or os.path.join(args.header_dir, "../generated")
    os.makedirs(output_dir, exist_ok=True)

    generated_files = []

    for header_file, header_functions in functions_by_header.items():
        # Get filename without extension
        header_name = os.path.splitext(os.path.basename(header_file))[0]
        module_name = header_name
        output_file = os.path.join(output_dir, f"{header_name}.generated.cpp")
        generated_files.append(output_file)
        
        # Calculate relative path from generated dir to header
        # From src/generated/ to src/public/test_script.h
        rel_path = os.path.relpath(header_file, output_dir)
        
        # Generate wrappers
        output = f"""

// Auto-generated Lua wrappers for {header_name}
// Include necessary headers
#include "{rel_path}"
#include "lua_bindings.h"

extern "C" {{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

// Lua wrapper functions

"""

        for func in header_functions:
            output += generate_lua_wrapper(func)

        output += generate_lua_module_registration(header_functions, module_name)
        output += "\n} // extern \"C\"\n"

        with open(output_file, 'w') as f:
            f.write(output)
        print(f"Generated {len(header_functions)} Lua wrappers for {header_name} in {output_file}")

    # Write generated files list for CMake
    cmake_file = os.path.join(output_dir, "generated_files.cmake")
    with open(cmake_file, 'w') as f:
        f.write("set(GENERATED_WRAPPERS\n")
        for gf in generated_files:
            abs_gf = os.path.abspath(gf).replace('\\', '/')
            f.write(f'    "{abs_gf}"\n')
        f.write(")\n")
    print(f"Wrote generated files list to {cmake_file}")

if __name__ == "__main__":
    main()