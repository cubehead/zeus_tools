if(NOT DEFINED ZEUS_CLI OR NOT EXISTS "${ZEUS_CLI}")
    message(FATAL_ERROR "ZEUS_CLI does not name the built CLI executable")
endif()

execute_process(
    COMMAND "${ZEUS_CLI}" --input json --action json.minify
            "${ZEUS_FIXTURES}/cli-json-input.json"
    RESULT_VARIABLE minify_status
    OUTPUT_VARIABLE minified
    ERROR_VARIABLE minify_error)
if(NOT minify_status EQUAL 0 OR
   NOT minified STREQUAL "{\"name\":\"Zeus\",\"active\":true}")
    message(FATAL_ERROR
        "CLI minify failed (${minify_status}): ${minify_error}; output=${minified}")
endif()

execute_process(
    COMMAND "${ZEUS_CLI}" --input json --action xml.to_json
            "${ZEUS_FIXTURES}/cli-json-input.json"
    RESULT_VARIABLE invalid_status
    OUTPUT_VARIABLE invalid_output
    ERROR_VARIABLE invalid_error)
if(invalid_status EQUAL 0 OR NOT invalid_error MATCHES "not available for JSON input")
    message(FATAL_ERROR "CLI should reject an inapplicable action: ${invalid_error}")
endif()

execute_process(
    COMMAND "${ZEUS_CLI}" --list-inputs
    RESULT_VARIABLE list_status
    OUTPUT_VARIABLE inputs
    ERROR_VARIABLE list_error)
if(NOT list_status EQUAL 0 OR NOT inputs MATCHES "(^|\n)unicode(\n|$)")
    message(FATAL_ERROR "CLI input listing failed: ${list_error}; output=${inputs}")
endif()

execute_process(
    COMMAND "${ZEUS_CLI}"
    INPUT_FILE "${ZEUS_FIXTURES}/cli-json-input.json"
    RESULT_VARIABLE stdin_status
    OUTPUT_VARIABLE stdin_output
    ERROR_VARIABLE stdin_error)
if(NOT stdin_status EQUAL 0 OR NOT stdin_output MATCHES "\"name\": \"Zeus\"")
    message(FATAL_ERROR "CLI stdin processing failed: ${stdin_error}; output=${stdin_output}")
endif()

execute_process(
    COMMAND "${ZEUS_CLI}" --input base64
            "${ZEUS_FIXTURES}/cli-base64-data-url.txt"
    RESULT_VARIABLE data_url_status
    OUTPUT_VARIABLE data_url_output
    ERROR_VARIABLE data_url_error)
if(NOT data_url_status EQUAL 0 OR NOT data_url_output STREQUAL "Hello, Zeus!")
    message(FATAL_ERROR
        "CLI Base64 Data URL failed (${data_url_status}): "
        "${data_url_error}; output=${data_url_output}")
endif()

get_filename_component(cli_directory "${ZEUS_CLI}" DIRECTORY)
set(oversized_path "${cli_directory}/zeus-cli-oversized-input.txt")
string(REPEAT "x" 10485761 oversized_input)
file(WRITE "${oversized_path}" "${oversized_input}")
execute_process(
    COMMAND "${ZEUS_CLI}" "${oversized_path}"
    RESULT_VARIABLE oversized_status
    OUTPUT_VARIABLE oversized_output
    ERROR_VARIABLE oversized_error)
file(REMOVE "${oversized_path}")
if(NOT oversized_status EQUAL 3 OR NOT oversized_error MATCHES "10 MiB limit")
    message(FATAL_ERROR "CLI should enforce the input boundary: ${oversized_error}")
endif()

set(malformed_toml_path "${cli_directory}/zeus-cli-malformed-toml.txt")
string(REPEAT "[" 256 malformed_toml_open)
string(REPEAT "]" 256 malformed_toml_close)
file(WRITE "${malformed_toml_path}"
    "${malformed_toml_open}${malformed_toml_close}")
execute_process(
    COMMAND "${ZEUS_CLI}" --input toml "${malformed_toml_path}"
    RESULT_VARIABLE malformed_toml_status
    OUTPUT_VARIABLE malformed_toml_output
    ERROR_VARIABLE malformed_toml_error)
file(REMOVE "${malformed_toml_path}")
if(NOT malformed_toml_status EQUAL 4 OR
   NOT malformed_toml_error MATCHES "PARSE_TOML" OR
   malformed_toml_error MATCHES "\\[\\[\\[")
    message(FATAL_ERROR
        "Malformed TOML should return a concise parse error without aborting or echoing input: "
        "status=${malformed_toml_status}; error=${malformed_toml_error}")
endif()
