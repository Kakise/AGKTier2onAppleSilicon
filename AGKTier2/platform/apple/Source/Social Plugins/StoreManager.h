
#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>
#import "StoreObserver.h"

@interface StoreManager : NSObject < SKProductsRequestDelegate >
{
	NSMutableArray*		purchasableObjects;
	StoreObserver*		storeObserver;
}

@property ( nonatomic, retain ) NSMutableArray* purchasableObjects;
@property ( nonatomic, retain ) StoreObserver*  storeObserver;

// public functions
+ ( void ) setup;
+ ( void ) setTitle: ( const char* ) ID;
+ ( void ) addProductID: ( const char* ) ID;
- ( void ) purchaseUnlockableContent:    ( int ) ID;
+ ( BOOL ) isUnlockableContentAvailable: ( int ) ID;
+ ( int  ) getState;

// private functions
- ( void ) requestProductData;
- ( void ) buyFeature: ( NSString* ) featureId;
- ( void ) failedTransaction: ( SKPaymentTransaction* ) transaction;
- ( void ) cancelledTransaction: ( SKPaymentTransaction* ) transaction;
- ( void ) provideContent: ( NSString* ) productIdentifier signature:(NSString*) signature;
+ ( StoreManager* ) sharedManager;
+ ( void ) loadPurchases;
+ ( void ) updatePurchases;
- ( void ) restore;
- ( void ) finishedRestore: (int) success;
- ( char*) getLocalPrice: (int) iID;
- ( char*) getDescription: (int) iID;
- ( char*) getSignature: (int) iID;

@end
