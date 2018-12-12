-- スレッドのテスト

-- スレッドID
local thread_id = 0

-- loadplugin時に呼ばれる
function OnInit()
  -- スレッド起動
  thread_id = coind.CreateThread("myThreadFunc")
end

-- unloadplugin時に呼ばれる
function OnTerm()
  -- スレッド終了待ち
  coind.Join(thread_id)
end

-- スリープ
function sleep(s)
  local ntime = os.clock() + s
  repeat until os.clock() > ntime
end

-- スレッド関数
function myThreadFunc()
  sleep(1)
  ret, value = coind.getbalance()
  if ret == true then
    print(string.format("[MyThread] getbalance : %f", value))
  end

  sleep(1)
  ret, tx = coind.sendtoaddress("p8Dp8vHVuCWVY474j6CEiSAPaFh1bgznaw", 1)
  if ret == true then
    print(string.format("[MyThread] sendtoaddress tx: %s", tx))
  end

  sleep(1)
  ret, value = coind.getbalance()
  if ret == true then
    print(string.format("[MyThread] getbalance : %f", value))
  end

end
