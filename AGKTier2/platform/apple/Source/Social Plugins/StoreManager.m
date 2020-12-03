
#import "StoreManager.h"
//#import "GameAppDelegate.h"

#import <UIKit/UIKit.h>

//extern int g_iPasses;

@implementation StoreManager

@synthesize purchasableObjects;
@synthesize storeObserver;


#define MAX_PRODUCTS 25

static int				productCount = 0;
static char				productID [ MAX_PRODUCTS ] [ 128 ];
static char				productPrice [ MAX_PRODUCTS ] [ 15 ];
static char				productDesc [ MAX_PRODUCTS ] [ 256 ];
static BOOL				purchasedProducts [ MAX_PRODUCTS ];
static char*			productSig [ MAX_PRODUCTS ];
static StoreManager*	_sharedStoreManager;
static char				title [ 128 ];
static int				state = 1;

+ ( void ) addProductID: ( const char* ) ID
{
    if ( productCount >= MAX_PRODUCTS )
    {
        NSLog(@"Max number of In App Purchase products reached");
        return;
    }
    
    if ( strlen(ID) > 127 )
    {
        NSLog(@"IAP product ID exceeds maximum length of 127 characters");
        return;
    }
	strcpy( productID[productCount++], ID );
}

+ ( int  ) getState
{
	return state;
}

+ ( void ) setTitle: ( const char* ) ID
{
	strcpy ( title, ID );
}

- ( void ) dealloc
{
	// this function is responsible for cleaning up
	
	[ _sharedStoreManager release ];
	[ storeObserver release ];
	[ super dealloc ];
}

+ ( BOOL ) isUnlockableContentAvailable: ( int ) ID
{
	// use this function to determine whether content is available
	
	//return YES;
	
	return purchasedProducts [ ID ];
}

+ ( void ) setup
{
	// this function will handle the initial set up

	@synchronized ( self )
	{
	    if ( _sharedStoreManager == nil )
		{
			for ( int i = 0; i < MAX_PRODUCTS; i++ )
            {
				purchasedProducts [ i ] = NO;
                memset( productPrice[ i ], 0, 15 );
                memset( productDesc[ i ], 0, 256 );
				productSig[i] = 0;
            }
			
			[ [ self alloc ] init ];
			_sharedStoreManager.purchasableObjects = [ [ NSMutableArray alloc ] init ];
			[ _sharedStoreManager requestProductData ];
			
			[ StoreManager loadPurchases ];
			_sharedStoreManager.storeObserver =  [ [ StoreObserver alloc ] init ];
			[ [ SKPaymentQueue defaultQueue ] addTransactionObserver: _sharedStoreManager.storeObserver ];
        }
    }
}

+ ( StoreManager* ) sharedManager
{
	// return the shared store manager

   return _sharedStoreManager;
}

+ ( id ) allocWithZone: ( NSZone* ) zone
{	
    @synchronized ( self )
	{
		if ( _sharedStoreManager == nil )
		{
			
            _sharedStoreManager = [ super allocWithZone: zone ];
            return _sharedStoreManager;
        }
    }
	
    return nil;
}

- ( id ) copyWithZone: ( NSZone* ) zone
{
    return self;
}

/*
- ( id ) retain
{	
    return self;	
}

- ( unsigned ) retainCount
{
    return UINT_MAX;
}

- ( void ) release
{

}
 */

- ( id ) autorelease
{
    return self;
}


- ( void ) requestProductData
{
    if ( productCount == 0 ) return;
    
	// this function is used to request data on the product IDs that we pass to the server
    NSString *str = 0;
    NSSet *set = [NSSet set];
    for ( int i = 0; i < productCount; i++ )
    {
        str = [ NSString stringWithUTF8String:productID [ i ] ];
        set = [ set setByAddingObject:str ];
    }
    
	// you can probably pass in an array here but for now just passing in each converted string one by one
	SKProductsRequest* request=  [ [ SKProductsRequest alloc ] initWithProductIdentifiers: set ];
    
    request.delegate = self;
	[ request start ];
}

- ( void ) productsRequest: ( SKProductsRequest* ) request didReceiveResponse: ( SKProductsResponse* ) response
{
	// this callback function will provide us with information from the product database

	[ purchasableObjects addObjectsFromArray:response.products ];
	
	for ( int i = 0; i < [ purchasableObjects count ]; i++ )
	{
		SKProduct* product = [ purchasableObjects objectAtIndex: i ];
		const char *szID = [product.productIdentifier UTF8String];
		
		int j = 0;
		for ( j = 0; j < productCount; j++ )
		{
			if ( strcmp( szID, productID[j] ) == 0 ) break;
		}
		if ( j == productCount ) continue;
        
        NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
        [numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
        [numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
        [numberFormatter setLocale:product.priceLocale];
        //[numberFormatter setLocale:[NSLocale localeWithLocaleIdentifier:@"fr_FR"]];
        numberFormatter.positiveSuffix = @"";
        numberFormatter.negativeSuffix = @"";

        
        NSString *currency_symbol = [numberFormatter currencySymbol];
        NSString *currency_code = [numberFormatter currencyCode];
        
        [numberFormatter setCurrencySymbol:@""];
        
        NSString *price = [numberFormatter stringFromNumber:product.price];
        
        @synchronized ( _sharedStoreManager )
        {
            // price
            if ( [currency_symbol canBeConvertedToEncoding:NSWindowsCP1252StringEncoding] )
            {
                strcpy( productPrice[j], [currency_symbol cStringUsingEncoding:NSWindowsCP1252StringEncoding] );
                strcat( productPrice[j], [price cStringUsingEncoding:NSWindowsCP1252StringEncoding] );
            }
            else
            {
                strcpy( productPrice[j], [price cStringUsingEncoding:NSWindowsCP1252StringEncoding] );
                strcat( productPrice[j], " " );
                strcat( productPrice[j], [currency_code cStringUsingEncoding:NSWindowsCP1252StringEncoding]);
            }
            
            // description
            NSData *data = [[product localizedDescription] dataUsingEncoding:NSWindowsCP1252StringEncoding allowLossyConversion:YES];
            if ( [data length] > 255 )
                strncpy( productDesc[j], (const char*)[data bytes], 255 );
            else
                strncpy( productDesc[j], (const char*)[data bytes], [data length] );
        }
		
		//NSLog ( @"Feature: %@, Symbol: %@, Code: %@, Price: %@, Cost: %@", [product localizedTitle], currency_symbol, currency_code, price, [NSString stringWithCString:productPrice[j] encoding:NSWindowsCP1252StringEncoding] );
        
        [ numberFormatter release ];
	}
	
	[ request autorelease ];
}

- (char*) getLocalPrice: (int) iID
{
    if ( _sharedStoreManager == nil || iID < 0 || iID >= productCount )
    {
        char *str = new char[1];
        *str = 0;
        return str;
    }
    else
    {
        @synchronized ( _sharedStoreManager )
        {
            char *str = new char[ strlen(productPrice[iID]) + 1 ];
            strcpy( str, productPrice[iID] );
            return str;
        }
    }
}

- (char*) getDescription: (int) iID
{
    if ( _sharedStoreManager == nil || iID < 0 || iID >= productCount )
    {
        char *str = new char[1];
        *str = 0;
        return str;
    }
    else
    {
        @synchronized ( _sharedStoreManager )
        {
            char *str = new char[ strlen(productDesc[iID]) + 1 ];
            strcpy( str, productDesc[iID] );
            return str;
        }
    }
}

- (char*) getSignature: (int) iID
{
	if ( _sharedStoreManager == nil || iID < 0 || iID >= productCount )
    {
        char *str = new char[1];
        *str = 0;
        return str;
    }
    else
    {
        @synchronized ( _sharedStoreManager )
        {
            char *str = new char[ strlen(productSig[iID]) + 1 ];
            strcpy( str, productSig[iID] );
            return str;
        }
    }
}

- ( void ) purchaseUnlockableContent: ( int ) ID
{
	// call this function when you want to purchase some new content

	state = 0;

	purchasedProducts [ ID ] = NO;

	NSString* pString = [ [ NSString alloc ] initWithUTF8String: productID [ ID ] ];

	[ self buyFeature: pString ];
    [pString release];
}

- ( void ) buyFeature: ( NSString* ) featureId
{
	// see if payments are available then add to queue or show an alert
	if ( [ SKPaymentQueue canMakePayments ] )
	{
		SKPayment* payment = [ SKPayment paymentWithProductIdentifier: featureId ];
		[ [ SKPaymentQueue defaultQueue ] addPayment: payment ];
	}
	else
	{
		NSString* pString = [ [ NSString alloc ] initWithUTF8String: productID [ 0 ] ];
			
		UIAlertView *alert = [ [ UIAlertView alloc ] initWithTitle:pString message:@"You are not authorized to purchase from the App Store"
													   delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil ];
		[ alert show ];
		[ alert release ];
        [pString release];
		
		state = 1;
	}
}

- ( void ) cancelledTransaction: ( SKPaymentTransaction* ) transaction
{
	state = 1;
}

- ( void ) failedTransaction: ( SKPaymentTransaction* ) transaction
{
	state = 1;

	
	NSString* messageToBeShown = [ NSString stringWithFormat:@"Please try again later." ];
	UIAlertView* alert = [ [ UIAlertView alloc ] initWithTitle:@"Unable to complete your purchase" message:messageToBeShown
												   delegate:self cancelButtonTitle:@"OK" otherButtonTitles: nil ];
	[ alert show ];
	[ alert release ];
}

- ( void ) restore
{
    state = 0;
    [[SKPaymentQueue defaultQueue]   restoreCompletedTransactions];
}

- ( void ) finishedRestore: (int) success
{
    NSLog(@"Restore Finished");
    state = 1;
}

- ( void ) provideContent: ( NSString* ) productIdentifier signature:(NSString*) signature
{
	state = 1;

	for ( int i = 0; i < productCount; i++ )
	{
		NSString* pString = [ [ NSString alloc ] initWithUTF8String: productID [ i ] ];
		
		if ( [ productIdentifier isEqualToString: pString ] )
		{
			purchasedProducts [ i ] = YES;

			if ( signature )
			{
				@synchronized ( _sharedStoreManager )
				{
					const char* szSig = [signature UTF8String];
					if ( productSig[ i ] ) delete [] productSig[ i ];
					productSig[ i ] = new char[ strlen(szSig) + 1 ];
					strcpy( productSig[i], szSig );
				}
			}
		}
        
        [pString release];
	}

	[ StoreManager updatePurchases ];
}

+ ( void ) loadPurchases 
{
	NSUserDefaults* userDefaults = [ NSUserDefaults standardUserDefaults ];
	
	for ( int i = 0; i < productCount; i++ )
	{
		NSString* pString = [ [ NSString alloc ] initWithUTF8String: productID [ i ] ];
	
		purchasedProducts [ i ] = [ userDefaults boolForKey:pString ];
        [pString release];
	}
}

+ ( void ) updatePurchases
{
	NSUserDefaults* userDefaults = [ NSUserDefaults standardUserDefaults ];
	
	for ( int i = 0; i < productCount; i++ )
	{
		NSString* pString = [ [ NSString alloc ] initWithUTF8String: productID [ i ] ];
	
		[ userDefaults setBool: purchasedProducts [ i ] forKey: pString ];
        [pString release];
	}
}
@end
