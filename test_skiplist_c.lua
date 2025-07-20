package.cpath = package.cpath .. ";./luaclib/?.so"
local skiplist = require "skiplist.c"

print("=== 测试skiplist.c模块 ===")

-- 测试空跳表
print("\n测试空跳表:")
local empty_sl = skiplist(0)
assert(empty_sl:obj_byrank(1) == nil, "空跳表查询应返回nil")
assert(empty_sl:rank_byobj(1, 100) == nil, "空跳表rank查询应返回nil")
local start_rank, end_rank = empty_sl:ranks_byscore(100, 100)
assert(start_rank == nil and end_rank == nil, "空跳表score查询应返回nil")

-- 测试升序模式(cmp=0)
print("\n测试升序模式(cmp=0):")
local sl0 = skiplist(0)  -- 升序

-- 插入数据
sl0:insert(1, 100)
sl0:insert(2, 50)
sl0:insert(3, 100)  -- 相同score
sl0:insert(4, 200)

-- 测试rank_byobj
print("\nrank_byobj测试:")
assert(sl0:rank_byobj(1, 100) == 2, "id=1 rank应为2")
assert(sl0:rank_byobj(2, 50) == 1, "id=2 rank应为1")
assert(sl0:rank_byobj(4, 200) == 4, "id=2 rank应为1")
assert(sl0:rank_byobj(99, 999) == nil, "不存在的obj应返回nil")

-- 测试ranks_byscore
print("\nranks_byscore测试:")
-- 测试无匹配情况
local s1, e1 = sl0:ranks_byscore(75, 75)
assert(s1 == nil, "75-75无匹配应返回nil")

-- 测试精确匹配
local s2, e2 = sl0:ranks_byscore(50, 50)
assert(s2 == 1 and e2 == 1, "50-50应返回[1,1]")

-- 测试范围查询
local s3, e3 = sl0:ranks_byscore(0, 75)
assert(s3 == 1 and e3 == 1, "0-75应返回[1,1]")

local s4, e4 = sl0:ranks_byscore(0, 100)
assert(s4 == 1 and e4 == 3, "0-100应返回[1,3]")

local s5, e5 = sl0:ranks_byscore(-1, 10000)
assert(s5 == 1 and e5 == 4, "0-100应返回[1,4]")


-- 测试obj_byrank
print("\nobj_byrank测试:")
print("rank=1 obj:", sl0:obj_byrank(1))
print("rank=3 obj:", sl0:obj_byrank(3))
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
assert(#sl0:objs_byrank(1, 10) == 4)
assert(#sl0:objs_byrank(5, 10) == 0, "超出范围的rank应返回空表")

-- 测试objs_byscore与ranks_byscore一致性
print("\n测试objs_byscore与ranks_byscore一致性:")
local min_score, max_score = 50, 150
local min_rank, max_rank = sl0:ranks_byscore(min_score, max_score)
print(string.format("score范围[%d,%d]对应的rank范围[%s,%s]", min_score, max_score, min_rank, max_rank))

local objs = sl0:objs_byscore(min_score, max_score)
local objs_by_rank = sl0:objs_byrank(min_rank, max_rank)

assert(#objs == #objs_by_rank, "两种方式获取的对象数量应一致")
for i = 1, #objs do
    assert(objs[i] == objs_by_rank[i], string.format("第%d个对象不匹配: %d != %d", i, objs[i], objs_by_rank[i]))
end

-- 测试边界条件
print("\n测试边界条件:")
-- 测试相等score
local same_score = 100
local same_start, same_end = sl0:ranks_byscore(same_score, same_score)
local same_objs = sl0:objs_byscore(same_score, same_score)
assert(#same_objs == (same_end - same_start + 1), "相同score范围的对象数量应匹配rank差值")

-- 测试降序模式(cmp=1)
print("\n测试降序模式(cmp=1):")
local sl1 = skiplist(1)  -- 降序

sl1:insert(1, 100)
sl1:insert(2, 50)
sl1:insert(3, 100)  -- 相同score
sl1:insert(4, 200)

print("\nranks_byscore测试(降序):")
local s3, e3 = sl1:ranks_byscore(75, 75)
print("score=75 rank:", s3)
local s4, e4 = sl1:ranks_byscore(150, 150)
print("score=150 rank:", s4)

print("\nobjs_byscore测试(降序):")
print("score 150-50:")
local objs = sl1:objs_byscore(150, 50)
for i, id in ipairs(objs) do
    print(i, id)
end
