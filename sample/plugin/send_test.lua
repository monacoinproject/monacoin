-- 送金コマンドテスト
-- テストの際は以下のアドレスを書き換えて下さい

-- sendfromテスト
function test_sendfrom()
  print("test_sendfrom:")

  ret, tx = coind.sendfrom("testaccount", "p8Dp8vHVuCWVY474j6CEiSAPaFh1bgznaw", 1)
  if ret == true then
    print(string.format("  tx: %s", tx))
  end
end


-- sendmany テスト
function test_sendmany()
  print("test_sendmany:")

  amounts = {p6Fy4dfgLHMuvyLvVLRBdKMobGEuNo9YTc = 1.0, p8Dp8vHVuCWVY474j6CEiSAPaFh1bgznaw = 1.0}
  amounts_json = cjson.encode(amounts)
  ret, tx = coind.sendmany("", amounts_json)
  if ret == true then
    print(string.format("  tx: %s", tx))
  end
end


-- sendtoaddress テスト
function test_sendtoaddress()
  print("test_sendtoaddress:")

  -- 引数省略もOK
  ret, tx = coind.sendtoaddress("p8Dp8vHVuCWVY474j6CEiSAPaFh1bgznaw", 1)
  -- ret, tx = coind.sendtoaddress("p8Dp8vHVuCWVY474j6CEiSAPaFh1bgznaw", 1, "comment", "comment_to", false, true, 2, "UNSET")

  if ret == true then
    print(string.format("  tx: %s", tx))
  end
end


-- loadplugin時に呼ばれる
function OnInit()
  -- どれか適当に
  test_sendfrom()
  -- test_sendmany()
  -- test_sendtoaddress()
end


-- 送金されるとOnWalletNotifyが発生する
-- ウォレットトランザクション通知（省略可）
function OnWalletNotify(txid)
  print(string.format("WalletNotiry:%s", os.date()))
  print(string.format("  Txid:%s", txid))

  ret, value = coind.gettransaction(txid)
  if ret == true then
     print(string.format("  blockhash:%s", value["blockhash"]))
     print(string.format("  blockindex:%s", value["blockindex"]))
     for i, val in pairs(value["details"]) do
       print(string.format("  [%s] address:%s  amount:%f", val["category"], val["address"], val["amount"]))
     end
  end
end
