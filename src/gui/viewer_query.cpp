/*
	*** Base Viewer - Object Querying
	*** src/gui/viewer_query.cpp
	Copyright T. Youngs 2013-2020

	This file is part of Dissolve.

	Dissolve is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Dissolve is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Dissolve.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "gui/dataviewer.hui"
#include "base/messenger.h"
#include <QOpenGLFramebufferObject>

// ViewerObject Keywords
const char* ViewerObjectKeywords[] = { "No Object", "Axis Line", "Axis Tick Label", "Axis Title Label", "Major Grid Line", "Minor Grid Line", "Renderable" };

// Return description of viewer object
const char* BaseViewer::viewerObject(BaseViewer::ViewerObject vo)
{
	return ViewerObjectKeywords[vo];
}

/*
 * Private Functions
 */

// Generate query image at specified scaling
void BaseViewer::generateQueryImage()
{
	// Flag that we are rendering offscreen
	renderingOffScreen_ = true;

	// Scale pixel-measures to reflect size of exported image
	setPixelScaling(queryImageScale_);

	// Make the offscreen surface the current context
	offscreenContext_.makeCurrent(&offscreenSurface_);

	// Initialise framebuffer format and object
	QOpenGLFramebufferObjectFormat fboFormat;
	fboFormat.setSamples(4);
	fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
	offscreenBuffer_ = new QOpenGLFramebufferObject(queryImageWidth_, queryImageHeight_, fboFormat);

	if (!offscreenBuffer_->bind())
	{
		Messenger::print("Failed to bind framebuffer object when generating query image.");
		return;
	}

	// Set the viewport of our View to be the new image size
	view_.recalculateViewport(queryImageWidth_, queryImageHeight_);

	// Generate this tile
	if (!offscreenBuffer_->bind()) printf("Failed to bind framebuffer object.\n");
	setupGL();
	renderGL();

	// TEST Save offscreen image
// 	QImage fboImage(offscreenBuffer_->toImage());
// 	QImage tile(fboImage.constBits(), fboImage.width(), fboImage.height(), QImage::Format_ARGB32);
// 	tile.save("query.png", "png");

	// Reset pixel scaling
	setPixelScaling(1.0);

	// Turn off offscreen rendering
	renderingOffScreen_ = false;

	// Reset the viewport of the View
	view_.recalculateViewport(contextWidth_, contextHeight_);

	// Reset context back to main view
	makeCurrent();

	// Delete the temporary buffer
	delete offscreenBuffer_;
}

/*
 * Protected Functions
 */

// Update object query, setting supplied information if the sample area has changed significantly
void BaseViewer::updateQuery(BaseViewer::ViewerObject objectType, const char* info, const char* subInfo)
{
	// Return immediately if we are not querying
	if ((!queryingObjects_) || (!offscreenBuffer_)) return;

	// Grab the current contents of the offscreen buffer as an image, not forgetting to re-bind it afterwards
	QImage fboImage(offscreenBuffer_->toImage());
	QImage tile(fboImage.constBits(), fboImage.width(), fboImage.height(), QImage::Format_ARGB32);
	offscreenBuffer_->bind();

	// Compare the stored colours in the region with those in the current buffer
	int index = 0;
	double delta = 0.0;
	for (int dx=0; dx<queryRegionWidth_; ++dx)
	{
		for (int dy=0; dy<queryRegionHeight_; ++dy)
		{
			// Accumulate difference between stored and current colour
			QColor pixelColour = tile.pixelColor(queryRegionLeft_+dx, queryImageHeight_ - queryRegionBottom_ - dy);
			delta += fabs(pixelColour.redF() - queryRegionR_[index]);
			delta += fabs(pixelColour.greenF() - queryRegionG_[index]);
			delta += fabs(pixelColour.blueF() - queryRegionB_[index]);

			// Store current colour
			queryRegionR_[index] = pixelColour.redF();
			queryRegionG_[index] = pixelColour.greenF();
			queryRegionB_[index] = pixelColour.blueF();

			++index;
		}
	}

	// Set the object info if the colour change threshold was reached
	const double threshold = 1.0; //(0.5 * queryRegionHeight_*queryRegionWidth_) * 0.25;
// 	printf("Delta = %f, threshold = %f\n", delta, threshold);
	if (delta > threshold)
	{
		queryObjectType_ = objectType;
		queryObjectInfo_ = info;
		queryObjectSubInfo_ = subInfo;
	}
}

/*
 * Public Functions
 */

// Return the type of object at the specified coordinates
BaseViewer::ViewerObject BaseViewer::queryAt(int x, int y)
{
	// Sampling size around central pixel
	const int sampleSize = 2;

	queryObjectType_ = BaseViewer::NoObject;
	queryObjectInfo_.clear();
	queryObjectSubInfo_.clear();

	// Set scale of query image and calculate size - we work in these coordinates from now on
	// BUG Setting query image scale to anything other than 1.0 results in slight differences in object positions, and inaccuracies in picking	
	queryImageScale_ = 1.0;
	queryImageWidth_ = contextWidth_ * queryImageScale_;
	queryImageHeight_ = contextHeight_ * queryImageScale_;
	x *= queryImageScale_;
	y *= queryImageScale_;

	// Check for invalid coordinates
	if ((x < 0) || (x >= queryImageWidth_) || (y < 0) || (y >= queryImageHeight_))
	{
		queryRegionLeft_ = -1;
		queryRegionBottom_ = -1;
		return BaseViewer::NoObject;
	}

	// Set up region to sample
	int left = x - sampleSize;
	int right = x + sampleSize;
	int top = y + sampleSize;
	int bottom = y - sampleSize;
	if (left < 0) left = 0;
	if (right >= queryImageWidth_) right = queryImageWidth_ - 1;
	if (top >= queryImageHeight_) top = queryImageHeight_ - 1;
	if (bottom < 0) bottom = 0;

	queryRegionLeft_ = left;
	queryRegionBottom_ = bottom;
	queryRegionWidth_ = (right - left) + 1;
	queryRegionHeight_ = (top - bottom) + 1;

	// Initialise the colour arrays
	queryRegionR_.initialise(queryRegionWidth_*queryRegionHeight_);
	queryRegionG_.initialise(queryRegionWidth_*queryRegionHeight_);
	queryRegionB_.initialise(queryRegionWidth_*queryRegionHeight_);
	queryRegionR_ = 1.0;
	queryRegionG_ = 1.0;
	queryRegionB_ = 1.0;

	// Flag that we are now querying, and generate a small offscreen image in order to perform the query
	queryingObjects_ = true;
	generateQueryImage();

	// Done
	queryingObjects_ = false;
	queryRegionLeft_ = -1;
	queryRegionBottom_ = -1;

	return queryObjectType_;
}

// Info for object at query coordinates
const char* BaseViewer::queryObjectInfo() const
{
	return queryObjectInfo_.get();
}

// Return sub-info for object at query coordinates
const char* BaseViewer::queryObjectSubInfo() const
{
	return queryObjectSubInfo_.get();
}

