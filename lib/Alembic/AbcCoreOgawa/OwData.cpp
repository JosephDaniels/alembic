//-*****************************************************************************
//
// Copyright (c) 2013,
//  Sony Pictures Imageworks, Inc. and
//  Industrial Light & Magic, a division of Lucasfilm Entertainment Company Ltd.
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// *       Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// *       Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// *       Neither the name of Sony Pictures Imageworks, nor
// Industrial Light & Magic nor the names of their contributors may be used
// to endorse or promote products derived from this software without specific
// prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//-*****************************************************************************

#include <Alembic/AbcCoreOgawa/CpwData.h>
#include <Alembic/AbcCoreOgawa/CpwImpl.h>
#include <Alembic/AbcCoreOgawa/OwData.h>
#include <Alembic/AbcCoreOgawa/OwImpl.h>
#include <Alembic/AbcCoreOgawa/WriteUtil.h>

namespace Alembic {
namespace AbcCoreOgawa {
namespace ALEMBIC_VERSION_NS {

//-*****************************************************************************
OwData::OwData( Ogawa::OGroupPtr iGroup ) : m_group( iGroup )
{
    // Check validity of all inputs.
    ABCA_ASSERT( m_group, "Invalid parent group" );

    m_data.reset( new CpwData( ".prop", m_group ) );
}

//-*****************************************************************************
OwData::~OwData()
{
    std::vector< uint8_t > data;

    // pack all object header into data here
    for ( size_t i = 0; i < m_childHeaders.size(); ++i )
    {
        WriteObjectHeader( data, m_childHeaders[i] );
    }

    if ( !data.empty() )
    {
        m_group->addData( data.size(), &( data.front() ) );
    }
}

//-*****************************************************************************
AbcA::CompoundPropertyWriterPtr
OwData::getProperties( AbcA::ObjectWriterPtr iParent )
{
    AbcA::CompoundPropertyWriterPtr ret = m_top.lock();
    if ( ! ret )
    {
        // time to make a new one
        ret.reset( new CpwImpl( iParent,
            m_data, iParent->getMetaData() ) );
        m_top = ret;
    }

    return ret;
}

//-*****************************************************************************
size_t OwData::getNumChildren()
{
    return m_childHeaders.size();
}

//-*****************************************************************************
const AbcA::ObjectHeader & OwData::getChildHeader( size_t i )
{
    if ( i >= m_childHeaders.size() )
    {
        ABCA_THROW( "Out of range index in OwImpl::getChildHeader: "
                     << i );
    }

    ABCA_ASSERT( m_childHeaders[i], "Invalid child header: " << i );

    return *(m_childHeaders[i]);
}

//-*****************************************************************************
const AbcA::ObjectHeader * OwData::getChildHeader( const std::string &iName )
{
    size_t numChildren = m_childHeaders.size();
    for ( size_t i = 0; i < numChildren; ++i )
    {
        if ( m_childHeaders[i]->getName() == iName )
        {
            return m_childHeaders[i].get();
        }
    }

    return NULL;
}

//-*****************************************************************************
AbcA::ObjectWriterPtr OwData::getChild( const std::string &iName )
{
    MadeChildren::iterator fiter = m_madeChildren.find( iName );
    if ( fiter == m_madeChildren.end() )
    {
        return AbcA::ObjectWriterPtr();
    }

    WeakOwPtr wptr = (*fiter).second;
    return wptr.lock();
}

AbcA::ObjectWriterPtr OwData::createChild( AbcA::ObjectWriterPtr iParent,
                                           const std::string & iFullName,
                                           const AbcA::ObjectHeader &iHeader )
{
    std::string name = iHeader.getName();

    if ( m_madeChildren.count( name ) )
    {
        ABCA_THROW( "Already have an Object named: "
                     << name );
    }

    if ( name.empty() )
    {
        ABCA_THROW( "Object not given a name, parent is: " <<
                    iFullName );
    }
    else if ( iHeader.getName().find('/') != std::string::npos )
    {
        ABCA_THROW( "Object has illegal name: "
                     << iHeader.getName() );
    }

    std::string parentName = iFullName;
    if ( parentName != "/" )
    {
        parentName += "/";
    }

    ObjectHeaderPtr header(
        new AbcA::ObjectHeader( iHeader.getName(),
                                parentName + iHeader.getName(),
                                iHeader.getMetaData() ) );

    AbcA::ObjectWriterPtr ret( new OwImpl( iParent,
                                           m_group->addGroup(),
                                           header ) );

    m_childHeaders.push_back( header );
    m_madeChildren[iHeader.getName()] = WeakOwPtr( ret );

    return ret;
}

} // End namespace ALEMBIC_VERSION_NS
} // End namespace AbcCoreOgawa
} // End namespace Alembic
