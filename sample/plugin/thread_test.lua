-- マルチスレッドのサンプル
-- スレッド２が後から実行されるが、スレッド１のUnlock～Sleep～Lockの間にスレッド２の処理が行われ、スレッド１より先に終了する。

-- スレッドID
local thread_1
local thread_2
local mutex

-- loadplugin時に呼ばれる
function OnInit()
  -- ミューテックス作成
  mutex = coind.CreateMutex()

  -- スレッド起動
  thread_1 = coind.CreateThread("myThreadFunc1")
  coind.Sleep(10);
  thread_2 = coind.CreateThread("myThreadFunc2")
end


-- unloadplugin時に呼ばれる
function OnTerm()
  -- スレッド終了待ち
  coind.Join(thread_1)
  coind.Join(thread_2)

  -- ミューテックス削除
  cond.DeleteMutex(mutex)
end


----------------------------------------------------------------------------
-- スレッド１
----------------------------------------------------------------------------
function myThreadFunc1()
  print("** Thread 1 Start **");

  coind.Lock(mutex);
    ret, value = coind.getbalance()
    if ret == true then
      print(string.format("  [Thread1] getbalance : %f", value))
    end
    coind.Sleep(1000);
  coind.Unlock(mutex);

  coind.Sleep(1000);

  coind.Lock(mutex);
    ret, value = coind.getbalance()
    if ret == true then
      print(string.format("  [Thread1] getbalance : %f", value))
    end
  coind.Unlock(mutex);

  print("** Thread 1 end **");
end

----------------------------------------------------------------------------
-- スレッド２
----------------------------------------------------------------------------
function myThreadFunc2()
  print("** Thread 2 Start **");

  coind.Lock(mutex);
    ret, tx = coind.sendtoaddress("p8Dp8vHVuCWVY474j6CEiSAPaFh1bgznaw", 1)
    if ret == true then
      print(string.format("  [Thread2] sendtoaddress tx: %s", tx))
    end
  coind.Unlock(mutex);

  print("** Thread 2 end **");
end

