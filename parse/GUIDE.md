# Parse

Parsing utilities for JSON and CSV data.

```stone
import parse
```

## JSON

`parse.json(string)` -- Parse JSON string, return raw value

`parse.json(string, Schema)` -- Parse with schema validation and defaults

```stone
User = { id: num, name: string, role: string = "user" }
user = parse.json('{"id": 1, "name": "Alice"}', User)
// user.role == "user" (default applied)
```

## CSV

`parse.csv(string)` -- Parse CSV into 2D string array

`parse.csv(string, Schema)` -- Parse with schema, returns array of records

`parse.csv(string, Schema, options)` -- With options: `delimiter`, `columns`

```stone
Point = { x: num, y: num }
points = parse.csv("x,y\n1,2\n3,4", Point)
```

## API Reference

| Function | Description |
|----------|-------------|
| `json(string)` | Parse JSON string into Stone value |
| `json(string, Schema)` | Parse JSON with typed schema validation |
