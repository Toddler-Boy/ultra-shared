#pragma once

#include <JuceHeader.h>

//-------------------------------------------------------------------------------------------------
// Browser-style wheel scrolling for a Viewport or ListBox, the viewport's whole-step wheel
// jump is rewound before it paints and replaced by a per-frame glide

class GUI_ViewportSmoothScroll final : public juce::MouseListener
{
public:
	// Bare viewport steps are small, default to twice the travel
	explicit GUI_ViewportSmoothScroll ( juce::Viewport& viewport, const float speedFactor = 2.0f ) : GUI_ViewportSmoothScroll ( viewport, viewport, speedFactor ) {}
	explicit GUI_ViewportSmoothScroll ( juce::ListBox& lb, const float speedFactor = 1.0f ) : GUI_ViewportSmoothScroll ( lb, *lb.getViewport (), speedFactor ) {}

	~GUI_ViewportSmoothScroll () override
	{
		comp.removeMouseListener ( this );
	}

	void mouseWheelMove ( const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel ) override
	{
		// Modified wheels keep their meaning, touch and trackpads already scroll per-pixel
		if ( e.mods.isAltDown () || e.mods.isCtrlDown () || e.mods.isCommandDown () )
			return;

		if ( wheel.isSmooth || wheel.isInertial || juce::approximatelyEqual ( wheel.deltaY, 0.0f ) )
			return;

		const auto&	scrollBar = vp.getVerticalScrollBar ();

		// The event's position is measured against a possibly recycled row, the source has the truth
		const auto	screenPos = e.source.getScreenPosition ().roundToInt ();

		// Header and scrollbar wheels scroll asynchronously via the scrollbar, leave them alone
		if ( ! vp.getScreenBounds ().contains ( screenPos )
			|| ( scrollBar.isVisible () && scrollBar.getScreenBounds ().contains ( screenPos ) ) )
			return;

		const auto	maxY = contentRange ();
		if ( maxY <= 0.0f )
			return;

		const auto	step = std::max ( 1.0f, float ( scrollBar.getSingleStepSize () ) );
		const auto	viewY = float ( vp.getViewPositionY () );

		// The viewport already jumped a whole step, nothing painted yet: rewind to the tracked
		// position if it plausibly is the jump's origin, else start from the view as it stands
		if ( ! animating )
		{
			const auto	jumped = viewY - position;
			const auto	stepLimit = wheelDistance * step * std::abs ( wheel.deltaY ) + 2.0f;

			if ( ! ( jumped * wheel.deltaY <= 0.0f && std::abs ( jumped ) <= stepLimit ) )
				position = viewY;

			position = juce::jlimit ( 0.0f, maxY, position );
			target = position;
			animating = true;
		}

		// Wheeling against the remaining glide restarts here, direction changes bite immediately
		const auto	delta = -wheel.deltaY * wheelDistance * step * speed;
		if ( ( target - position ) * delta < 0.0f )
			target = position;

		target = juce::jlimit ( 0.0f, maxY, target + delta );

		writeViewPosition ();
	}

private:
	// Native-step travel per wheel unit, ease-out covers ~95% of the way in 0.25 s
	static constexpr auto	wheelDistance = 14.0f;
	static constexpr auto	easeRate = 12.0f;

	GUI_ViewportSmoothScroll ( juce::Component& eventSource, juce::Viewport& viewport, const float speedFactor )
		: comp ( eventSource ), vp ( viewport ), speed ( speedFactor )
	{
		comp.addMouseListener ( this, true );
	}

	void update ( const double frameSec )
	{
		const auto	dt = float ( juce::jlimit ( 0.0, 0.1, frameSec - lastFrameSec ) );
		lastFrameSec = frameSec;

		if ( ! animating )
		{
			position = float ( vp.getViewPositionY () );
			return;
		}

		// Someone else scrolled mid-glide (keys, a programmatic scroll), they win
		if ( vp.getViewPositionY () != juce::roundToInt ( position ) )
		{
			animating = false;
			return;
		}

		target = juce::jlimit ( 0.0f, contentRange (), target );

		position += ( target - position ) * ( 1.0f - std::exp ( -easeRate * dt ) );

		if ( std::abs ( target - position ) < 0.5f )
		{
			position = target;
			animating = false;
		}

		writeViewPosition ();
	}

	[[ nodiscard ]] float contentRange () const
	{
		const auto*	content = vp.getViewedComponent ();
		if ( content == nullptr )
			return 0.0f;

		return float ( std::max ( 0, content->getHeight () - vp.getViewHeight () ) );
	}

	void writeViewPosition ()
	{
		vp.setViewPosition ( vp.getViewPositionX (), juce::roundToInt ( position ) );
	}

	juce::Component&	comp;
	juce::Viewport&		vp;
	const float			speed;

	float	position = 0.0f;
	float	target = 0.0f;
	double	lastFrameSec = 0.0;
	bool	animating = false;

	juce::VBlankAttachment	vblank { &comp, [ this ] ( const double frameSec ) { update ( frameSec ); } };

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR ( GUI_ViewportSmoothScroll )
};
//-------------------------------------------------------------------------------------------------
