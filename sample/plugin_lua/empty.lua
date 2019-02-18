-- デフォルトで呼ばれるコールバックの説明
-- 全て省略可

-- loadplugin時に呼ばれる関数
function OnInit()
end

-- unloadplugin時に呼ばれる関数
function OnTerm()
end

-- ブロック更新通知
-- initialsync (bool) 初期ダウンロード中であるか
-- hash (string) ブロックハッシュ
function OnBlockNotify(initialsync, hash)
end

-- ウォレットトランザクション通知
-- txid (string) トランザクションID
function OnWalletNotify(txid)
end

