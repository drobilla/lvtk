local import = require ('lvtk.import')
local exec = import.exec

local bundle = import ('lvtk.lv2')
assert (type(bundle) == 'table', "Failed to import bundle")
exec (bundle, 'lvtk.Demo')
