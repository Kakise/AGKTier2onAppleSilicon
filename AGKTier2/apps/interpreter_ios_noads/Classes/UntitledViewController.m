//
//  UntitledViewController.m
//  Untitled
//
//  Created by Paul Johnston on 02/12/2010.
//  Copyright 2010 __MyCompanyName__. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>

#import "UntitledViewController.h"
#include "AGK.h"
#include "interpreter.h"

// define one of these to fix the player to that orientation forever
//#define AGK_LANDSCAPE
//#define AGK_PORTRAIT

using namespace AGK;

// Uniform index.
enum {
    UNIFORM_TRANSLATE,
    NUM_UNIFORMS
};
GLint uniforms[NUM_UNIFORMS];

// Attribute index.
enum {
    ATTRIB_VERTEX,
    ATTRIB_COLOR,
    NUM_ATTRIBUTES
};

BOOL g_bDisplayLinkReady = FALSE;

@interface UntitledViewController ()
@property (nonatomic, retain) EAGLContext *context;
@end

@implementation UntitledViewController

@synthesize context;

- (void)didReceiveMemoryWarning
{
    // doesn't recognise memory pool?
    //NSLog( @"Received memory warning" );
    //extern NSAutoreleasePool *pool;
    //[pool drain];
    [super didReceiveMemoryWarning];
}

- (void)awakeFromNib
{
    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.view.layer;
    eaglLayer.opaque = TRUE;
    eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                    [NSNumber numberWithBool:FALSE], kEAGLDrawablePropertyRetainedBacking,
                                    kEAGLColorFormatRGBA8, kEAGLDrawablePropertyColorFormat,
                                    nil];
    
    appActive = 0;
    
    agk::SetExtraAGKPlayerAssetsMode ( 2 );
    agk::SetResolutionMode(1);
    try
    {
        agk::InitGL( self );
    }
    catch(...)
    {
        uString err; agk::GetLastError(err);
        err.Prepend( "Error: " );
        agk::Message( err.GetStr() );
        PlatformAppQuit();
        return;
    }
        
    active = FALSE;
    frameInterval = 1;
    displayLink = nil;
    g_bDisplayLinkReady = TRUE;
}

-(BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation
{
#ifdef AGK_LANDSCAPE
    return (interfaceOrientation == UIInterfaceOrientationLandscapeLeft) || (interfaceOrientation == UIInterfaceOrientationLandscapeRight);
#else
#ifdef AGK_PORTRAIT
    return (interfaceOrientation == UIInterfaceOrientationPortrait) || (interfaceOrientation == UIInterfaceOrientationPortraitUpsideDown);
#else
    if ( agk::IsCapturingImage() && ([[UIDevice currentDevice] userInterfaceIdiom] != UIUserInterfaceIdiomPad) )
    {
        return interfaceOrientation == UIInterfaceOrientationPortrait;
    }
    
    switch( interfaceOrientation )
    {
        case UIInterfaceOrientationPortrait: return agk::CanOrientationChange(1) ? YES : NO;
        case UIInterfaceOrientationPortraitUpsideDown: return agk::CanOrientationChange(2) ? YES : NO;
        case UIInterfaceOrientationLandscapeLeft: return agk::CanOrientationChange(4) ? YES : NO;
        case UIInterfaceOrientationLandscapeRight: return agk::CanOrientationChange(3) ? YES : NO;
        default: return NO;
    }
#endif
#endif
}

-(NSUInteger)supportedInterfaceOrientations
{
#ifdef AGK_LANDSCAPE
    return UIInterfaceOrientationMaskLandscape;
#else
#ifdef AGK_PORTRAIT
    return UIInterfaceOrientationMaskPortrait | UIInterfaceOrientationMaskPortraitUpsideDown;
#else
    int result = 0;
    if ( agk::CanOrientationChange(1) ) result |= UIInterfaceOrientationMaskPortrait;
    if ( agk::CanOrientationChange(2) ) result |= UIInterfaceOrientationMaskPortraitUpsideDown;
    if ( agk::CanOrientationChange(3) ) result |= UIInterfaceOrientationMaskLandscapeRight;
    if ( agk::CanOrientationChange(4) ) result |= UIInterfaceOrientationMaskLandscapeLeft;
    return result;
#endif
#endif
}

-(BOOL)shouldAutoRotate
{
    return YES;
}

-(BOOL)prefersHomeIndicatorAutoHidden
{
    // set by SetImmersiveMode
    return agk::GetInternalDataI(0) ? YES : NO;
}

-(void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    //NSLog(@"Orientation Changed");
    agk::UpdatePtr( self );
}

- (BOOL) prefersStatusBarHidden
{
    return YES;
}

- (void)dealloc
{
    [super dealloc];
}

- (void)viewWillAppear:(BOOL)animated
{
    // do this in app delegate instead
    //[self setActive];
    
    [super viewWillAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    // do this in app delegate instead
    //[self setInactive];
    
    [super viewWillDisappear:animated];
}

- (void)viewDidUnload
{
    [super viewDidUnload];
}

- (NSInteger)frameInterval
{
    return frameInterval;
}

- (void)setFrameInterval:(NSInteger)interval
{
    /*
     Frame interval defines how many display frames must pass between each time the display link fires.
     The display link will only fire 30 times a second when the frame internal is two on a display that refreshes 60 times a second. The default frame interval setting of one will fire 60 times a second when the display refreshes at 60 times a second. A frame interval setting of less than one results in undefined behavior.
     */
    if (interval >= 1)
    {
        frameInterval = interval;
        
        if (active)
        {
            [self setInactive];
            [self setActive];
        }
    }
}

- (void)setActive
{
    if ( g_bDisplayLinkReady == FALSE )
        return;
    
    if (!active)
    {
        displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector(drawView)];
        [displayLink setFrameInterval:frameInterval];
        [displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSRunLoopCommonModes];
        
        active = TRUE;
    }
}

- (void)setInactive
{
    if (active)
    {
        [displayLink invalidate];
        displayLink = nil;
        
        active = FALSE;
    }
}

- (void)setAppActive: (int) mode
{
    appActive = mode;
}

- (void)drawView
{
    static int has_error = 0;
    if ( has_error ) return;
    if ( appActive == 0 ) return;
    
    if ( agk::m_iChartboostIsDisplaying ) return;
    
    try
    {
        App.Loop();
    }
    catch(...)
    {
        uString err; agk::GetLastError(err);
        err.Prepend( "Error: \n\n" );
        agk::Message( err.GetStr() );
        has_error = 1;
    }
}

@end
