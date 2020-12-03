package com.thegamecreators.agk_player.iap;

import java.util.HashSet;
import java.util.Map;
import java.util.Set;

import android.util.Log;

import com.amazon.device.iap.PurchasingListener;
import com.amazon.device.iap.PurchasingService;
import com.amazon.device.iap.model.FulfillmentResult;
import com.amazon.device.iap.model.Product;
import com.amazon.device.iap.model.ProductDataResponse;
import com.amazon.device.iap.model.PurchaseResponse;
import com.amazon.device.iap.model.PurchaseUpdatesResponse;
import com.amazon.device.iap.model.Receipt;
import com.amazon.device.iap.model.UserDataResponse;
import com.thegamecreators.agk_player.AGKHelper;

/**
 * Implementation of {@link PurchasingListener} that listens to Amazon
 * InAppPurchase SDK's events, and call {@link AGKIapManager} to handle the
 * purchase business logic.
 */
public class AGKPurchasingListener implements PurchasingListener {

    private static final String TAG = "IAP Listener";

    public AGKPurchasingListener() {

    }

    /**
     * This is the callback for {@link PurchasingService#getUserData}. For
     * successful case, get the current user from {@link UserDataResponse} and
     * call {@link AGKIapManager#setAmazonUserId} method to load the Amazon
     * user and related purchase information
     * 
     * @param response
     */
    @Override
    public void onUserDataResponse(final UserDataResponse response) {
        Log.d(TAG, "onGetUserDataResponse: requestId (" + response.getRequestId()
                   + ") userIdRequestStatus: "
                   + response.getRequestStatus()
                   + ")");
    }

    /**
     * This is the callback for {@link PurchasingService#getProductData}. After
     * SDK sends the product details and availability to this method, it will set the purchase
     * status accordingly.
     */
    @Override
    public void onProductDataResponse(final ProductDataResponse response) {
        final ProductDataResponse.RequestStatus status = response.getRequestStatus();
        //Log.d(TAG, "onProductDataResponse: RequestStatus (" + status + ")");

        switch (status) {
        case SUCCESSFUL:
            //Log.d(TAG, "onProductDataResponse: successful.  The item data map in this response includes the valid SKUs");
            final Map<String, Product> products = response.getProductData();
            //Log.i( TAG, products.toString() );
            if ( !response.getUnavailableSkus().isEmpty() ) Log.w( TAG, "Invalid IAP SKUs: " + response.getUnavailableSkus().toString() );
            //for (Map.Entry<String,Product> entry : products.entrySet())
            for ( int i = 0; i < AGKHelper.g_iNumProducts; i++ )
            {
                Product item = products.get( AGKHelper.g_sPurchaseProductNames[i] );
                if ( item != null )
                {
                    //Log.i( TAG, "Got " + AGKHelper.g_sPurchaseProductNames[i] + ": " + item.toString() );

                    String price = item.getPrice();
                    char symbol = price.charAt(0);
                    String numbers = "0123456789.,";
                    int index = 0;
                    while( index < price.length() && !numbers.contains(price.substring(index,index+1)) ) index++;
                    if ( index == 0 ) symbol = price.charAt(price.length()-1);
                    price = price.substring(index);
                    index = price.length()-1;
                    while( index > 0 && !numbers.contains(price.substring(index,index+1)) ) index--;
                    price = price.substring(0,index+1);

                    switch( symbol )
                    {
                        case '$': price = "$" + price; break;
                        case '£': price = "p" + price; break; // can't transfer pound character to AGK easily, so use a place holder and replace it in AGK
                        case '€': price = "e" + price; break; // can't transfer euro character to AGK easily, so use a place holder and replace it in AGK
                        default: Log.w(TAG, "Unrecognised currency: " + symbol);
                    }

                    synchronized (AGKHelper.iapLock) {
                        AGKHelper.g_sPurchaseProductPrice[i] = price;
                        AGKHelper.g_sPurchaseProductDesc[i] = AGKHelper.ConvertString(item.getDescription());
                    }

                    Log.i( TAG, "SKU Details for " + AGKHelper.g_sPurchaseProductNames[i]
                              + " Desc: " + AGKHelper.g_sPurchaseProductDesc[i]
                              + ", Price Raw: " + item.getPrice()
                              + ", Price: " + AGKHelper.g_sPurchaseProductPrice[i]);
                }
            }

            AGKHelper.g_iIAPStatus = 2;

            // get previous purchases
            PurchasingService.getPurchaseUpdates(true);
            break;

        case FAILED:
        case NOT_SUPPORTED:
            Log.d(TAG, "onProductDataResponse: failed, should retry request");
            AGKHelper.g_iIAPStatus = -1;
            break;
        }
    }

    /**
     * This is the callback for {@link PurchasingService#getPurchaseUpdates}.
     * 
     * We will receive Consumable receipts from this callback if the consumable
     * receipts are not marked as "FULFILLED" in Amazon Appstore. So for every
     * single Consumable receipts in the response, we need to call
     * {@link AGKIapManager#handleReceipt} to fulfill the purchase.
     * 
     */
    @Override
    public void onPurchaseUpdatesResponse(final PurchaseUpdatesResponse response) {

        //Log.d(TAG, "onPurchaseUpdatesResponse: " + response.getReceipts().toString());

        final PurchaseUpdatesResponse.RequestStatus status = response.getRequestStatus();
        switch (status) {
        case SUCCESSFUL:
            for (final Receipt receipt : response.getReceipts()) {
                if ( !receipt.isCanceled() ) {
                    for (int i = 0; i < AGKHelper.g_iNumProducts; i++) {
                        if (receipt.getSku().equals(AGKHelper.g_sPurchaseProductNames[i])) {
                            Log.i(TAG, "Update: Restored " + AGKHelper.g_sPurchaseProductNames[i]);
                            AGKHelper.g_iPurchaseProductStates[i] = 1;
                            AGKHelper.g_iPurchaseState = 1;
                            PurchasingService.notifyFulfillment(receipt.getReceiptId(), FulfillmentResult.FULFILLED);
                            break;
                        }
                    }
                }
            }
            if (response.hasMore()) {
                PurchasingService.getPurchaseUpdates(false);
            }
            break;
        case FAILED:
        case NOT_SUPPORTED:
            Log.d(TAG, "onProductDataResponse: failed, should retry request");
            break;
        }

    }

    /**
     * This is the callback for {@link PurchasingService#purchase}. For each
     * time the application sends a purchase request
     * {@link PurchasingService#purchase}, Amazon Appstore will call this
     * callback when the purchase request is completed. If the RequestStatus is
     * Successful or AlreadyPurchased then application needs to call
     * to handle the purchase
     * fulfillment. If the RequestStatus is INVALID_SKU, NOT_SUPPORTED, or
     * FAILED, notify corresponding method of .
     */
    @Override
    public void onPurchaseResponse(final PurchaseResponse response) {
        final String requestId = response.getRequestId().toString();
        final String userId = response.getUserData().getUserId();
        final PurchaseResponse.RequestStatus status = response.getRequestStatus();
        Receipt receipt = response.getReceipt();

        //Log.d(TAG, "onPurchaseResponse: " + receipt.toString());

        if ( AGKHelper.g_iPurchaseState != 0 )
        {
            Log.w( TAG, "Received purchase receipt when purchase is not in progress" );
            PurchasingService.notifyFulfillment(receipt.getReceiptId(), FulfillmentResult.FULFILLED);
            return;
        }

        switch (status) {
            case SUCCESSFUL:
            Log.d(TAG, "onPurchaseResponse: receipt json:" + receipt.toJSON());
            switch (receipt.getProductType())
            {
                case CONSUMABLE:
                    // try to do your application logic to fulfill the customer purchase
                    if (receipt.isCanceled())
                    {
                        // revoke?
                    }
                    else
                    {
                        for ( int i = 0; i < AGKHelper.g_iNumProducts; i++ )
                        {
                            if ( receipt.getSku().equals(AGKHelper.g_sPurchaseProductNames[i]) )
                            {
                                Log.i(TAG, "Bought " + AGKHelper.g_sPurchaseProductNames[i]);
                                AGKHelper.g_iPurchaseProductStates[i] = 1;
                                PurchasingService.notifyFulfillment(receipt.getReceiptId(), FulfillmentResult.FULFILLED);
                                break;
                            }
                        }
                    }
                    break;
                case ENTITLED:
                    // check entitled sample app to know how to handle entitled
                    // purchases
                    if (receipt.isCanceled())
                    {
                        // revoke?
                    }
                    else
                    {
                        for ( int i = 0; i < AGKHelper.g_iNumProducts; i++ )
                        {
                            if ( receipt.getSku().equals( AGKHelper.g_sPurchaseProductNames[i] ) )
                            {
                                Log.i(TAG, "Bought " + AGKHelper.g_sPurchaseProductNames[i]);
                                AGKHelper.g_iPurchaseProductStates[i] = 1;
                                PurchasingService.notifyFulfillment(receipt.getReceiptId(), FulfillmentResult.FULFILLED);
                                break;
                            }
                        }
                    }
                    break;
                case SUBSCRIPTION:
                    // check subscription sample app to know how to handle subscription
                    // purchases
                    break;
            }
            break;
        case ALREADY_PURCHASED:
            Log.d(TAG, "onPurchaseResponse: already purchased: " + receipt.toString());
            for ( int i = 0; i < AGKHelper.g_iNumProducts; i++ )
            {
                if ( receipt.getSku().equals(AGKHelper.g_sPurchaseProductNames[i]) )
                {
                    Log.i(TAG, "Restored " + AGKHelper.g_sPurchaseProductNames[i]);
                    AGKHelper.g_iPurchaseProductStates[i] = 1;
                    PurchasingService.notifyFulfillment(receipt.getReceiptId(), FulfillmentResult.FULFILLED);
                    break;
                }
            }
            break;
        case INVALID_SKU:
            Log.d(TAG, "onPurchaseResponse: invalid SKU!");
            break;
        case FAILED:
        case NOT_SUPPORTED:
            Log.d(TAG, "onPurchaseResponse: failed");
            break;
        }

        AGKHelper.g_iPurchaseState = 1;
    }

}
