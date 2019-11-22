### Platform Support

The following shows module APIs available for each platform.

|  | Linux<br/>(Ubuntu) | Raspbian<br/>(Raspberry Pi) | NuttX<br/>(STM32F4-Discovery) | TizenRT<br/>(Artik053) |
| :---: | :---: | :---: | :---: | :---: |
| require | O | O | O | O |

# Module
The `require` function is always available there is no need to import `module` explicitly.

### require(id)
* `id` {string} Module name to be loaded.

Loads the module named `id`.

**Example**

```js
var assert = require('assert');

assert.equal(2, 2);
```

### require.cache
* {object} Module map to be loaded.

Modules are cached in this object when they are required. By deleting a key value from this object, the next require will reload the module.

**Loading a module**

If a native module named `id` exists, load it and return.
(_Native module:_ which module came from the IoT.js itself)

`require` function searches for modules in the following order:

1. `$NODE_PRIORITIZED_PATH` if present.
2. `node_modules` folder under current working directory.
3. `$NODE_PATH` if present.
4. `$HOME/node_modules`.

For each directory in search paths above:

- If a file `id` exists, load it and return.
- If a file `id.js` exists, load it and retun.
- If a directory `id` exists, module system consider the directory as a package:
  - If `id/package.json` contains **main** property, load the file named **main** property.
  - If `id/package.json` exists, but neither the **main** property nor the file named **main** property exist, load `index.js`.

### require.main
* accessing the main module

When a file is run directly, `require.main` is set to its module. That means that it is possible to determine whether a file has been run directly by testing `require.main === module`. For a file foo.js, this will be true if run via cmd directly, but false if run by `require('./foo')`, because `module` object is set to the `foo` module but `require.main` is still your entry file module. So you can get the entry point of the current application from checking `require.main.filename` anywhere.

