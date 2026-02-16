set(json [=[{
  "name": "test",
  "version": {"major": 3, "minor": 14, "patch": 0},
  "features": ["feat1", "feat2", "feat3"],
  "config": {
    "debug": true,
    "optimize": false,
    "level": 2,
    "paths": ["/usr/local", "/opt"]
  },
  "empty_obj": {},
  "null_val": null
}]=])
string(JSON val GET "${json}" "name")
string(JSON val GET "${json}" "version" "major")
string(JSON val GET "${json}" "features" 0)
string(JSON val GET "${json}" "features" 2)
string(JSON val GET "${json}" "config" "debug")
string(JSON val GET "${json}" "config" "paths" 1)
string(JSON type TYPE "${json}" "name")
string(JSON type TYPE "${json}" "version")
string(JSON type TYPE "${json}" "features")
string(JSON type TYPE "${json}" "config" "debug")
string(JSON type TYPE "${json}" "null_val")
string(JSON len LENGTH "${json}" "features")
string(JSON len LENGTH "${json}" "config")
string(JSON member MEMBER "${json}" 0)
string(JSON member MEMBER "${json}" "config" 0)
string(JSON out SET "${json}" "new_key" "\"new_value\"")
string(JSON out SET "${json}" "version" "patch" 1)
string(JSON out SET "${json}" "features" 3 "\"feat4\"")
string(JSON out REMOVE "${json}" "null_val")
string(JSON out REMOVE "${json}" "config" "debug")
string(JSON val ERROR_VARIABLE err GET "${json}" "nonexistent")
message(STATUS "error: ${err}")
