using System;
using System.IO;


// クラス名"MonaPlugin"の中の下記のメソッドがmonacondから呼び出されます
class MonaPlugin
{
    // プラグインのロードが成功した場合に最初に呼ばれる
    public void OnInit()
    {
        Console.WriteLine("****** OnInit *****");
    }

    // プラグインが削除される直前に呼ばれる
    public void OnTerm()
    {
        Console.WriteLine("****** OnTerm *****");
    }

    // 自分のアドレスに関わる通知
    public void OnWalletNotify(string hash)
    {
        Console.WriteLine("****** OnWalletNotify *****");
        Console.WriteLine("hash : {0}", hash);
    }

    // ブロックが更新されたときの通知
    public void OnBlockNotify(bool binit, string hash)
    {
        if(!binit)
        {
            Console.WriteLine("****** OnBlockNotify *****");
            Console.WriteLine("hash : {0}", hash);
        }
    }
}
