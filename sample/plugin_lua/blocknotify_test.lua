-- OnBlockNotify のテスト

-- ブロック更新通知
function OnBlockNotify(initialsync, hash)
  print(string.format("BlockNotiry:%s", os.date()))
  print(string.format("  BlockHash:%s", hash))
  print(string.format("  Initial Sync:%s", initialsync and "true" or "false"))
  ret, value = coind.getblock(hash)
  if ret == true then
    print(string.format("  Height:%d", value["height"]))
    print(string.format("  Difficulty:%f", value["difficulty"]))
    print(string.format("  nTx:%d", value["nTx"]))
  end
end

