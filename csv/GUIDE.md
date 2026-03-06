# CSV Parsing

```stone
import parse from csv
```

`parse(string)` — Parse CSV into 2D string array

`parse(string, Schema)` — Parse with schema-driven type conversion, returns array of records

`parse(string, Schema, options)` — Parse with schema + options

Options: `delimiter` (field separator, default ","), `columns` (positional column-to-field mapping for headerless data)

```stone
import parse from csv

// Raw 2D string array
data = parse("x,y\n1,2\n3,4")

// With schema (each row becomes a record)
Point = { x: num, y: num }
points = parse(csv_text, Point)
// -> [{x = 1, y = 2}, {x = 3, y = 4}]

// Tab-separated, no header
data = parse(text, Point, {delimiter = "\t", columns = ["x", "y"]})
```
