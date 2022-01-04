#include "RectPacking.h"

#include <algorithm>

namespace Packing
{
    
    // ***********************************************************************

    struct SortByHeight
    {
        bool operator()(const Rect& a, const Rect& b)
        {
            return a.h > b.h;
        }
    };

    // ***********************************************************************

    struct SortToOriginalOrder
    {
        bool operator()(const Rect& a, const Rect& b)
        {
            return a.ordering < b.ordering;
        }
    };

    // ***********************************************************************

    void Packing::RowPackRects(std::vector<Rect>& rects, int width, int height)
    {
        for (int i = 0; i < rects.size(); i++)
        {
            rects[i].ordering = i;
        }

        std::sort(rects.begin(), rects.end(), SortByHeight());

        int xPos = 0;
        int yPos = 0;

        int largestHThisRow = 0;

        // Pack from left to right on a row
        int maxX = 0;
        int maxY = 0;
        int totalArea = 0;

        for (Rect& rect : rects)
        {
            if ((xPos + rect.w) > width)
            {
                yPos += largestHThisRow;
                xPos = 0;
                largestHThisRow = 0;
            }

            if ((yPos + rect.h) > height)
                break;

            rect.x = xPos;
            rect.y = yPos;

            xPos += rect.w;
            
            if (rect.h > largestHThisRow)
                largestHThisRow = rect.h;

            rect.wasPacked = true;
        }

        std::sort(rects.begin(), rects.end(), SortToOriginalOrder());
    }

    // ***********************************************************************
    
    struct SkylineNode
    {
        int x, y, width;
    };

    int CanRectFit(std::vector<SkylineNode>& nodes, int atNode, int rectWidth, int rectHeight, int width, int height)
    {
        // See if there's space for this rect at node "atNode"

        int x = nodes[atNode].x;
        int y = nodes[atNode].y;
        if (x + rectWidth > width) // Check we're not going off the end of the image
            return -1;

        
        // We're going to loop over all the nodes from atNode to however many this new rect "covers"
        // We want to find the highest rect under neath this rect to place it at.
        int remainingSpace = rectWidth;
        int i = atNode;
        while (remainingSpace > 0)
        {
            SkylineNode& node = nodes[i];

            if (i == nodes.size()) return -1;

            if (node.y > y)
                y = node.y;

            if (y + rectHeight > height) return -1; // of the edge of the image
            remainingSpace -= node.width;
            i++;
        }
        return y;
    }
    
    // ***********************************************************************

    void Packing::SkylinePackRects(std::vector<Rect>& rects, int width, int height)
    {
        for (int i = 0; i < rects.size(); i++)
        {
            rects[i].ordering = i;
        }

        // Sort by a heuristic
        std::sort(rects.begin(), rects.end(), SortByHeight());

        int maxX = 0;
        int maxY = 0;
        int totalArea = 0;
        
        std::vector<SkylineNode> nodes;

        nodes.push_back({0, 0, width});

        for (Rect& rect : rects)
        {
            int bestHeight = height;
            int bestWidth = width;
            int bestNode = -1;
            int bestX, bestY;
            // We're going to search for the best location for this rect along the skyline
            for(int i = 0; i < nodes.size(); i++)
            {
                SkylineNode& node = nodes[i];
                int highestY = CanRectFit(nodes, i, rect.w, rect.h, width, height);
                if (highestY != -1)
                {
                    // Settling a tie here on best height by checking lowest width we can use up
                    if (highestY + rect.h < bestHeight || (highestY + rect.h == bestHeight && node.width < bestWidth))
                    {
                        bestNode = i;
                        bestWidth = node.width;
                        bestHeight = highestY + rect.h;
                        bestX = node.x;
                        bestY = highestY;
                    }
                }
            }

            if (bestNode == -1)
                continue; // We failed

            // Add the actual rect, changing the skyline

            // First add the new node
            SkylineNode newNode;
            newNode.width = rect.w;
            newNode.x = bestX;
            newNode.y = bestY + rect.h;
            nodes.insert(nodes.begin() + bestNode, newNode);

            // Now we have to find all the nodes underneath that new skyline level and remove them
            for(int i = bestNode+1; i < nodes.size(); i++)
            {
                SkylineNode& node = nodes[i];
                SkylineNode& prevNode = nodes[i - 1];
                // Check to see if the current node is underneath the previous node
                // Remember that i starts as the first node after we inserted, so the previous node is the one we inserted
                if (node.x < prevNode.x + prevNode.width) 
                {
                    // Draw a picture of this
                    int amountToShrink = (prevNode.x + prevNode.width) - node.x;
                    node.x += amountToShrink;
                    node.width -= amountToShrink;

                    if (node.width <= 0) // We've reduced this nodes with so much it can be removed
                    {
                        nodes.erase(nodes.begin() + i);
                        i--; // Move back since we've removed a node
                    }
                    else
                    {
                        break; // If we don't need to remove this node we've reached the extents of our new covering node
                    }
                }
                else
                {
                    break; // Nothing being covered
                }
            }

            // Find any skyline nodes that are the same height and remove them
            for(int i = 0; i < nodes.size() - 1; i++)
            {
                if (nodes[i].y == nodes[i + 1].y)
                {
                    nodes[i].width += nodes[i + 1].width;
                    nodes.erase(nodes.begin() + (i + 1));
                    i--;
                }
            }

            rect.x = bestX;
            rect.y = bestY;

            rect.wasPacked = true;
        }
        std::sort(rects.begin(), rects.end(), SortToOriginalOrder());
    }

}