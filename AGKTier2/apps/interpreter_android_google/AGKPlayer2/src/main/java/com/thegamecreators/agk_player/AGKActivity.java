package com.thegamecreators.agk_player;

import android.Manifest;
import android.app.Activity;
import android.app.NativeActivity;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.graphics.Bitmap;
import android.hardware.display.DisplayManager;
import android.media.MediaRecorder;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.provider.MediaStore;
import androidx.core.content.ContextCompat;
import android.util.DisplayMetrics;
import android.util.Log;
import android.widget.Toast;

import com.google.android.gms.auth.api.signin.GoogleSignIn;
import com.google.android.gms.auth.api.signin.GoogleSignInAccount;
import com.google.android.gms.auth.api.signin.GoogleSignInStatusCodes;
import com.google.android.gms.common.api.ApiException;
import com.google.android.gms.tasks.Task;
import com.google.android.youtube.player.YouTubeInitializationResult;
import com.google.android.youtube.player.YouTubeStandalonePlayer;

import java.io.FileOutputStream;
import java.io.IOException;

public class AGKActivity extends NativeActivity
{
    @Override
    public void onCreate( Bundle state )
    {
        super.onCreate( state );

        Intent intent = getIntent();
        if ( intent != null )
        {
            Uri data = intent.getData();
            if ( data != null ) AGKHelper.g_sLastURI = data.toString();
        }
    }

    @Override
    public void onNewIntent( Intent intent )
    {
        if ( intent == null ) return;
        if ( intent.getData() == null ) return;
        AGKHelper.g_sLastURI = intent.getData().toString();
    }

    @Override
    public void onActivityResult( int requestCode, int resultCode, Intent data )
    {
        switch( requestCode ) {
            case 9000:
            case 9002: { // InAppBilling
                if (!AGKHelper.mHelper.handleActivityResult(requestCode, resultCode, data)) {
                    Log.e("IAP", "Failed to handle activity result " + resultCode);
                } else {
                    Log.i("IAP", "Result code handled by IABUtil: " + resultCode);
                }
                break;
            }
            case 10003: // Google Drive sign in
            {
                Task<GoogleSignInAccount> task = GoogleSignIn.getSignedInAccountFromIntent(data);
                try {
                    AGKHelper.g_GoogleAccount = task.getResult(ApiException.class);
                    AGKHelper.FinishCloudDataSetup(task);
                } catch (ApiException e) {
                    Log.e("Google Sign In", "Failed to sign in, error code: " + e.getStatusCode());
                    if (e.getStatusCode() == GoogleSignInStatusCodes.SIGN_IN_CANCELLED)
                        AGKHelper.g_iCloudDataStatus = -1;
                    else if (e.getStatusCode() != GoogleSignInStatusCodes.SIGN_IN_CURRENTLY_IN_PROGRESS)
                        AGKHelper.g_iCloudDataStatus = -2;
                }
                break;
            }
            case 10004: // Google Games sign in
            {
                Task<GoogleSignInAccount> task = GoogleSignIn.getSignedInAccountFromIntent(data);
                try {
                    AGKHelper.g_GamesAccount = task.getResult(ApiException.class);
                    AGKHelper.GameCenterCompleteLogin( this );
                } catch (ApiException e) {
                    Log.e("Games Sign In", "Failed to sign in, error code: " + e.getStatusCode());
                    if (e.getStatusCode() == GoogleSignInStatusCodes.SIGN_IN_CANCELLED)
                        AGKHelper.m_GameCenterLoggedIn = -1;
                    else if (e.getStatusCode() != GoogleSignInStatusCodes.SIGN_IN_CURRENTLY_IN_PROGRESS)
                        AGKHelper.m_GameCenterLoggedIn = -1;
                    AGKHelper.g_GamesAccount = null;
                    AGKHelper.g_GamesSignIn = null;
                }
                break;
            }
            case 9003: // youtube start video playback
            {
                if ( resultCode != RESULT_OK ) {
                    YouTubeInitializationResult errorReason = YouTubeStandalonePlayer.getReturnedInitializationResult(data);
                    if (errorReason.isUserRecoverableError()) {
                        errorReason.getErrorDialog(this, 0).show();
                    } else {
                        String errorMessage = errorReason.toString();
                        Toast.makeText(this, errorMessage, Toast.LENGTH_LONG).show();
                    }
                }
                break;
            }
            case 9004: // capture camera image
            {
                if ( resultCode == RESULT_OK )
                {
                    if (data != null && data.getExtras() != null) {
                        Bitmap imageBitmap = (Bitmap) data.getExtras().get("data");
                        try {
                            FileOutputStream out = new FileOutputStream(AGKHelper.sCameraSavePath);
                            imageBitmap.compress(Bitmap.CompressFormat.JPEG, 95, out);
                            Log.w("Camera Image", "Saved image to: " + AGKHelper.sCameraSavePath);
                            AGKHelper.iCapturingImage = 2;
                            return;
                        }
                        catch( IOException e )
                        {
                            Log.e("Camera Image", "Failed to save image: "+e.toString() );
                        }
                    }
                    AGKHelper.iCapturingImage = 0;
                }
                else
                {
                    Log.e("Camera Image", "User cancelled capture image" );
                    AGKHelper.iCapturingImage = 0;
                }
                break;
            }
            case 9005: // choose image
            {
                if ( resultCode == RESULT_OK )
                {
                    if (data != null) {
                        Uri uri = data.getData();
                        try {
                            Bitmap imageBitmap = MediaStore.Images.Media.getBitmap(getContentResolver(), uri);
                            FileOutputStream out = new FileOutputStream(AGKHelper.sChosenImagePath);
                            imageBitmap.compress(Bitmap.CompressFormat.JPEG, 95, out);
                            Log.w("Choose Image", "Saved image to: " + AGKHelper.sChosenImagePath);
                            AGKHelper.iChoosingImage = 2;
                            return;
                        }
                        catch( IOException e )
                        {
                            Log.e("Choose Image", "Failed to save image: "+e.toString() );
                        }
                    }
                    AGKHelper.iChoosingImage = 0;
                }
                else
                {
                    Log.e("Choose Image", "User cancelled choose image" );
                    AGKHelper.iChoosingImage = 0;
                }
                break;
            }
        }
    }

    public void onRequestPermissionsResult(int requestCode, String permissions[], int[] grantResults)
    {
        int index = requestCode-5;
        if ( index < 0 ) return;
        if ( index >= AGKHelper.g_sPermissions.length ) return;

        AGKHelper.g_iPermissionStatus[index] = 2;
    }
}