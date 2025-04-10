# Table Applies

This document describes the new "table_applies" feature in BMv2, which allows the same table to be applied multiple times in a single pipeline with different next nodes.

## Background

In the original BMv2 JSON format, each table has a "next_tables" field that specifies the next node to execute after the table. This means that a table can only be applied once in a pipeline, or if it's applied multiple times, all applications must have the same next nodes.

## New JSON Format

The new JSON format introduces a "table_applies" field in the pipeline, which contains a list of table application nodes. Each table application node has:

- "name": A unique name for the table application
- "id": A unique ID for the table application
- "table": The name of the table to apply
- "next_tables": The next nodes to execute after the table, just like the original "next_tables" field in tables

Example:

```json
"table_applies": [
  {
    "name": "ipv4_lpm_apply1",
    "id": 1,
    "table": "ipv4_lpm",
    "next_tables": {
      "ipv4_forward": "check_ipv4",
      "drop": null,
      "NoAction": null
    }
  },
  {
    "name": "ipv4_lpm_apply2",
    "id": 2,
    "table": "ipv4_lpm",
    "next_tables": {
      "ipv4_forward": null,
      "drop": null,
      "NoAction": null
    }
  }
]
```

In this example, the "ipv4_lpm" table is applied twice in the pipeline, with different next nodes for each application.

## Version Handling

The new JSON format uses version [2, 25] to indicate that it supports table_applies. BMv2 will continue to support the old format for backward compatibility.

## Performance Optimization

For tables that are only applied once in a pipeline, BMv2 will continue to use the existing optimization where the next node is cached in the table entry. For tables that are applied multiple times, BMv2 will use this optimization for one of the applications and determine the next node at runtime for the other applications.

## Implementation Details

The implementation adds a new `TableApply` class that inherits from `ControlFlowNode` and represents a single application of a table in a pipeline. The `TableApply` class has a reference to the table and its own set of next nodes.

The `MatchTableAbstract` class has been modified to support multiple table applies, with a `cached_table_apply` field that indicates which table apply is using the cached next node values.

The `P4Objects` class has been modified to parse the new JSON format and create the appropriate `TableApply` objects.

## Example

See the `testdata/table_applies_example.json` file for a complete example of the new JSON format.

## Testing

The implementation includes unit tests for the new functionality:

- `test_table_applies.cpp`: Tests the core functionality of the `TableApply` class
- `test_json_table_applies.cpp`: Tests the parsing of the new JSON format and the execution of a pipeline with multiple table applies
