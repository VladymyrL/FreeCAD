/***************************************************************************
 *   Copyright (c) Yorik van Havre          (yorik@uncreated.net) 2013     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
#endif

#include <iomanip>
#include <iterator>
#include <boost/regex.hpp>

#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Console.h>

#include "DrawViewSymbol.h"

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewSymbol
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewSymbol, TechDraw::DrawView)


DrawViewSymbol::DrawViewSymbol(void)
{
    static const char *vgroup = "Drawing view";

    ADD_PROPERTY_TYPE(Symbol,(""),vgroup,App::Prop_Hidden,"The SVG code defining this symbol");
    ADD_PROPERTY_TYPE(EditableTexts,(""),vgroup,App::Prop_None,"Substitution values for the editable strings in this symbol");
    ScaleType.setValue("Custom");
}

DrawViewSymbol::~DrawViewSymbol()
{
}

void DrawViewSymbol::onChanged(const App::Property* prop)
{
    if (prop == &Symbol) {
        if (!isRestoring()) {
            std::vector<string> eds;
            std::string svg = Symbol.getValue();
            if (!svg.empty()) {
                boost::regex e ("<text.*?freecad:editable=\"(.*?)\".*?<tspan.*?>(.*?)</tspan>");
                std::string::const_iterator tbegin, tend;
                tbegin = svg.begin();
                tend = svg.end();
                boost::match_results<std::string::const_iterator> twhat;
                while (boost::regex_search(tbegin, tend, twhat, e)) {
                    eds.push_back(twhat[2]);
                    tbegin = twhat[0].second;
                }
                EditableTexts.setValues(eds);
            }
        }
    }
    TechDraw::DrawView::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawViewSymbol::execute(void)
{
    std::string svg = Symbol.getValue();
    const std::vector<std::string>& editText = EditableTexts.getValues();

    if (!editText.empty()) {
        //TODO: has this ever been run?
        boost::regex e1 ("<text.*?freecad:editable=\"(.*?)\".*?<tspan.*?>(.*?)</tspan>");
        string::const_iterator begin, end;
        begin = svg.begin();
        end = svg.end();
        boost::match_results<std::string::const_iterator> what;
        std::size_t count = 0;
        std::string newsvg;
        newsvg.reserve(svg.size());

        while (boost::regex_search(begin, end, what, e1)) {
            if (count < editText.size()) {
                // change values of editable texts. Also strip the "freecad:editable"
                // attribute so it isn't detected by the page
                boost::regex e2 ("(<text.*?)(freecad:editable=\""+what[1].str()+"\")(.*?<tspan.*?)>(.*?)(</tspan>)");
                std::back_insert_iterator<std::string> out(newsvg);
                boost::regex_replace(out, begin, what[0].second, e2, "$1$3>"+editText[count]+"$5");
            }
            count++;
            begin = what[0].second;
        }

        // now copy the rest
        newsvg.insert(newsvg.end(), begin, end);
        svg = newsvg;
    }
    //TODO: shouldn't there be a Symbol.setValue(svg) here??? -wf
    return DrawView::execute();
}

QRectF DrawViewSymbol::getRect() const
{
        std::string svg = Symbol.getValue();
        double w = 64.0;         //must default to something
        double h = 64.0;
        string::const_iterator begin, end;
        begin = svg.begin();
        end = svg.end();
        boost::match_results<std::string::const_iterator> what;

        boost::regex e1 ("width=\"([0-9.]*?)[a-zA-Z]*?\"");
        if (boost::regex_search(begin, end, what, e1)) {
            //std::string wText = what[0].str();             //this is the whole match 'width="100"'
            std::string wNum  = what[1].str();               //this is just the number 100
            w = std::stod(wNum);
        }
        boost::regex e2 ("Height=\"([0-9.]*?)[a-zA-Z]*?\"");
        if (boost::regex_search(begin, end, what, e1)) {
            //std::string hText = what[0].str();
            std::string hNum  = what[1].str();
            h = std::stod(hNum);
        }
        return (QRectF(0,0,Scale.getValue() * w,Scale.getValue() * h));
}


// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewSymbolPython, TechDraw::DrawViewSymbol)
template<> const char* TechDraw::DrawViewSymbolPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderSymbol";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewSymbol>;
}
