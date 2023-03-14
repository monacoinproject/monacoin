package org.monacoincore.qt;

import android.os.Bundle;
import android.system.ErrnoException;
import android.system.Os;

import org.qtproject.qt5.android.bindings.QtActivity;

import java.io.File;

public class MonacoinQtActivity extends QtActivity
{
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        final File monacoinDir = new File(getFilesDir().getAbsolutePath() + "/.monacoin");
        if (!monacoinDir.exists()) {
            monacoinDir.mkdir();
        }

        super.onCreate(savedInstanceState);
    }
}
