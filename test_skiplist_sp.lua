package.cpath = package.cpath .. ";./luaclib/?.so"
local skiplist = require "skiplist.sp"

print("=== 测试skiplist模块 ===")

-- 测试空跳表
print("\n测试空跳表:")
local empty_sl = skiplist(0, 0)
assert(empty_sl:obj_byrank(1) == nil, "空跳表查询应返回nil")
assert(empty_sl:rank_byobj(1, 100, 1) == nil, "空跳表rank查询应返回nil")
local start_rank, end_rank = empty_sl:ranks_byscore(100,0, 100,0)
assert(start_rank == nil and end_rank == nil, "空跳表score查询应返回nil")

-- 测试升序模式(cmp0=0, cmp1=0)
print("\n测试升序模式(cmp0=0, cmp1=0):")
local sl0 = skiplist(0, 0)  -- 主score升序，时间score升序

-- 插入数据(score1, score2=时间戳)
sl0:insert(1, 100, 1)  -- id=1, score1=100, time=1
sl0:insert(2, 50, 2)   -- id=2, score1=50, time=2
sl0:insert(3, 100, 3)  -- 相同score1，不同时间
sl0:insert(4, 200, 4)

-- 测试rank_byobj
print("\nrank_byobj测试:")
assert(sl0:rank_byobj(1, 100, 1) == 2, "id=1 rank应为2")
assert(sl0:rank_byobj(3, 100, 3) == 3, "id=3 rank应为3")
assert(sl0:rank_byobj(99, 999, 99) == nil, "不存在的obj应返回nil")

-- 测试obj_byrank
print("\nobj_byrank测试:")
assert(sl0:obj_byrank(1) == 2, "rank=1 obj应为2")
assert(sl0:obj_byrank(3) == 3, "rank=3 obj应为3")
assert(sl0:obj_byrank(0) == nil, "rank=0应返回nil")
assert(sl0:obj_byrank(5) == nil, "超过最大rank应返回nil")

-- 测试objs_byrank
print("\nobjs_byrank测试:")
print("rank 1-3:")
local objs = sl0:objs_byrank(1, 3)
assert(#objs == 3, "应返回3个对象")
for i, id in ipairs(objs) do
    print(i, id)
    assert(id == 2 or id == 1 or id == 3, "返回的id应在预期范围内")
end
assert(#sl0:objs_byrank(5, 10) == 0, "超出范围的rank应返回空表")

-- 测试objs_byscore
print("\nobjs_byscore测试:")
print("score (50,0)-(150,0):")
local objs = sl0:objs_byscore(50,0, 150, math.maxinteger)
assert(#objs == 3, "应返回3个对象")
for i, id in ipairs(objs) do
    print(i, id)
    assert(id == 2 or id == 1 or id == 3, "返回的id应在预期范围内")
end
assert(#sl0:objs_byscore(300,0, 250, math.maxinteger) == 0, "超出范围的score应返回空表")

-- 测试ranks_byscore
print("\nranks_byscore测试:")
-- 测试无匹配情况
local s1, e1 = sl0:ranks_byscore(75,0, 75, math.maxinteger)
assert(s1 == nil, "75-75无匹配应返回nil")

-- 测试精确匹配
local s2, e2 = sl0:ranks_byscore(50,0, 50, math.maxinteger)
print(s2, e2)
assert(s2 == 1 and e2 == 1, "50-50应返回[1,1]")

-- 测试范围查询
local s3, e3 = sl0:ranks_byscore(0,0, 75,math.maxinteger)
assert(s3 == 1 and e3 == 1, "0-75应返回[1,1]")

local s4, e4 = sl0:ranks_byscore(0,0, 100,math.maxinteger)
assert(s4 == 1 and e4 == 3, "0-100应返回[1,3]")

-- 验证与objs_byscore结果一致
local objs = sl0:objs_byscore(0,0, 100, math.maxinteger)
assert(#objs == (e4 - s4 + 1), "ranks_byscore与objs_byscore结果不一致")

-- 验证与objs_byscore结果一致
local objs = sl0:objs_byscore(50,0, 150, math.maxinteger)
local start_rank, end_rank = sl0:ranks_byscore(50,0, 150, math.maxinteger)
assert(#objs == (end_rank - start_rank + 1), "ranks_byscore与objs_byscore结果不一致")

-- 测试降序模式(cmp0=1, cmp1=0)
print("\n测试降序模式(cmp0=1, cmp1=0):")
local sl1 = skiplist(1, 0)  -- 主score降序，时间score升序

sl1:insert(1, 100, 1)
sl1:insert(2, 50, 2)
sl1:insert(3, 100, 3)  -- 相同score1，不同时间
sl1:insert(4, 200, 4)

print("\nobjs_byscore测试(降序):")
print("score (150,0)-(50,0):")
local objs = sl1:objs_byscore(150,0, 50, math.maxinteger)
for i, id in ipairs(objs) do
    print(i, id)
end

-- 测试ranks_byscore(降序)
print("\nranks_byscore测试(降序):")
print("score (150,0)-(50,math.maxinteger):")
local start_rank, end_rank = sl1:ranks_byscore(150, 0, 50, math.maxinteger)
print("start_rank:", start_rank, "end_rank:", end_rank)
assert(start_rank == 2 and end_rank == 4, "rank范围应为2-4")

print("score (100,0)-(75,math.maxinteger):")
start_rank, end_rank = sl1:ranks_byscore(100,0, 50, math.maxinteger)
print("start_rank:", start_rank, "end_rank:", end_rank)
assert(start_rank == 2 and end_rank == 4, "rank范围应为2-3")

-- 测试时间排序(cmp0=0, cmp1=1)
print("\n测试时间排序(cmp0=0, cmp1=1):")
local sl2 = skiplist(0, 1)  -- 主score升序，时间score降序

sl2:insert(1, 100, 1)
sl2:insert(2, 100, 2)  -- 相同score1，更晚时间
sl2:insert(3, 100, 3)
sl2:insert(4, 200, 4)

print("\n相同score按时间降序:")
local objs = sl2:objs_byrank(1, 4)
assert(#objs == 4, "应返回4个对象")
for i, id in ipairs(objs) do
    print(i, id)
    if i <= 3 then
        assert(id == 3-i+1, "时间降序验证失败")
    end
end
