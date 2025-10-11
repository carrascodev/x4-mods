local f = package.loadlib('D:\\dev\\x4\\build\\Release\\nakama_x4.dll','luaopen_nakama_x4_test')
local m = f()
print('module:', m, m.status)
print('ping ->', m.ping())
