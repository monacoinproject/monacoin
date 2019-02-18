//
// RPCコマンドを呼び出すサンプル
// CallCoindCommand にメソッド名とパラメーターを渡すと結果が返る
// OnBlockNotify では返ってきたjsonをパースして表示している
//


using System;
using System.IO;
using System.Json;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

class MonaPlugin
{
    // プラグインのロードが成功した場合に最初に呼ばれる
    public void OnInit()
    {
        Console.WriteLine("****** OnInit *****");

        // 適当にコマンドを実行してみる
        // コマンド、引数はRPCと同じ
        // （パラメーターはテスト時のものなので適宜書き換えてください）
        string res;
        res = CallCoindCommand("sendfrom", "testasccount", "p88ZwicTpUKyiC7JR9gAVm6S81aSQhDT2z", 1.1);
        Console.WriteLine("sendfrom : {0}", res);

        res = CallCoindCommand("getpeerinfo");
        Console.WriteLine("getpeerinfo : {0}", res);

        res = CallCoindCommand("getbalance", "testaccount");
        Console.WriteLine("getbalance : {0}", res);
    }

    // ブロックが更新されたときの通知
    public void OnBlockNotify(bool binit, string hash)
    {
        if(!binit)
        {
            Console.WriteLine("****** OnBlockNotify *****");

            // getblockコマンドで戻ってきたjsonをパース
            var json = JsonObject.Parse(CallCoindCommand("getblock", hash));

            // いくつか出力
            Console.WriteLine("hash : {0}", json["hash"]);
            Console.WriteLine("confirmations : {0}", json["confirmations"]);
            Console.WriteLine("size : {0}", json["size"]);
            Console.WriteLine("height : {0}", json["height"]);
            Console.WriteLine("tx [");
            foreach(var tx in json["tx"])
            {
                Console.WriteLine(tx);
            }
            Console.WriteLine("]");
        }
    }


    //----------------------------------------------------------
    // CallCoindCommand
    // coindのコマンドを呼び出す関数
    // 
    // _method : メソッド名
    // _params : パラメーター配列
    // 返り値 : コマンドの実行結果
    //----------------------------------------------------------

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    public static extern string CoindCommand(string _method, string _param);

    public string CallCoindCommand(string _method, params object[] _params)
    {
        string strParams = "";
        if(_params.Length != 0)
        {
            foreach(var p in _params)
            {
                if(p.GetType().Equals(typeof(string)))
                {
                    strParams += ",\"" + p.ToString() + "\"";
                }
                else
                {
                    strParams += "," + p.ToString();
                }
            }
            strParams = "[" + strParams.Substring(1) + "]";
        }
        else
        {
            strParams = "[]";
        }

        return CoindCommand(_method, strParams);
    }
}
