// ORIG-DATE:     Dec 2007
// -*- Mode : c++ -*-
//
// SUMMARY  :  Model  mesh 2d   
// USAGE    : LGPL      
// ORG      : LJLL Universite Pierre et Marie Curi, Paris,  FRANCE 
// AUTHOR   : Frederic Hecht
// E-MAIL   : frederic.hecht@ann.jussieu.fr
//

/*
 
 This file is part of Freefem++
 
 Freefem++ is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.
 
 Freefem++  is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with Freefem++; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 
 Thank to the ARN ()  FF2A3 grant
 ref:ANR-07-CIS7-002-01
 */

#include <fstream>
#include <iostream>
#include <cstring>
#include "libmesh5.h"
#include "ufunction.hpp"
#include "error.hpp"
#include "RNM.hpp"
namespace Fem2D
{
}
#include "Mesh2dn.hpp"
#include "Mesh3dn.hpp"
#include "MeshSn.hpp"
#include "MeshLn.hpp"
#include "rgraph.hpp"
#include "fem.hpp"
#include "PlotStream.hpp"

namespace Fem2D
{
    
    template<> int   GenericMesh<TriangleS,BoundaryEdgeS,Vertex3>::kfind=0;
    template<> int   GenericMesh<TriangleS,BoundaryEdgeS,Vertex3>::kthrough=0;
  
    
    const string GsbeginS="MeshS::GSave v0",GsendS="end";
    void MeshS::GSave(FILE * ff,int offset) const
    {
        PlotStream f(ff);
        
        f <<  GsbeginS ;
        f << nv << nt << nbe;
        for (int k=0; k<nv; k++) {
            const  Vertex & P = this->vertices[k];
            f << P.x <<P.y << P.z << P.lab ;
        }
        
        for (int k=0; k<nt; k++) {
            const Element & K(this->elements[k]);
            int i0=this->operator()(K[0])+offset;
            int i1=this->operator()(K[1])+offset;
            int i2=this->operator()(K[2])+offset;
            int lab=K.lab;
            f << i0 << i1 << i2  << lab;
        }
        
        for (int k=0; k<nbe; k++) {
            const BorderElement & K(this->borderelements[k]);
            int i0=this->operator()(K[0])+offset;
            int i1=this->operator()(K[1])+offset;
            int lab=K.lab;
            f << i0 << i1  << lab;
        }
        f << GsendS;
    }
    
    
    
    void  MeshS::read(istream &f)
    { // read the mesh
        int i;
        //    f >> nv >> nt >> neb ;
        string str;
        int err=0;
        while(!f.eof())
        {
            f >> str;
            //cout << str << endl;
            if( str== "Vertices")
            {
                f >> nv;
                assert(!this->vertices );
                if(verbosity>2)
                    cout << "  -- Nb of Vertex " << nv << endl;
                this->vertices = new Vertex[nv];
                for (i=0;i<nv;i++)
                {
                    f >> this->vertices[i];
                    assert(f.good());
                }
            }
            else if (str=="Triangles")
            {
                f >> nt;
                assert(this->vertices && !this->elements);
                this->elements = new Element [nt];
                mes=0;
                assert(this->elements);
                if(verbosity>2)
                    cout <<   "  -- Nb of Elements " << nt << endl;
                for (int i=0;i<nt;i++)
                {
                    this->t(i).Read1(f,this->vertices,this->nv);
                    if(this->t(i).mesure()<=0) err++; // Modif FH nov 2014
                    mes += this->t(i).mesure();
                }
            }
            else if (str=="Edges")
            {
                mesb=0;
                int kmv=0,ij;
                f >> nbe;
                assert(vertices);
                this->borderelements = new BorderElement[nbe];
                if(verbosity>2)
                    cout <<   "  -- Nb of border Triangles " << nbe << endl;
                for (i=0;i<nbe;i++)
                {
                    this->be(i).Read1(f,this->vertices,this->nv);
                    mesb += this->be(i).mesure();
                    for(int j=0;j<BorderElement::nv;++j)
                        if(!vertices[ij=this->be(i,j)].lab)
                        {
                            vertices[ij].lab=1;
                            kmv++;
                        }
                }
            }
            else if(str[0]=='#')
            {// on mange la ligne
                int c;
                while ( (c=f.get()) != '\n' &&  c != EOF)
                    //cout << c << endl;
                    ;
            }
        }
        assert( (nt >= 0 || nbe>=0)  && nv>0) ;
        /*   done at up level ...
         BuildBound();
         
         if(nt > 0){
         BuildAdj();
         Buildbnormalv();
         BuildjElementConteningVertex();
         }
         */
        if(err!=0)
        {
            cerr << " MeshS::read: sorry bad mesh. Number of negative Triangles " << err << endl;
            this->~MeshS();
            ffassert(0);
        }
    }
    
    
    void MeshS::readmsh(ifstream & f,int offset)
    {
        int err=0;
        f >> nv >> nt >> nbe;
        if(verbosity>2)
            cout << " GRead : nv " << nv << " " << nt << " " << nbe << endl;
        this->vertices = new Vertex[nv];
        this->elements = new Element [nt];
        this->borderelements = new BorderElement[nbe];
        for (int k=0; k<nv; k++) {
            Vertex & P = this->vertices[k];
            f >> P.x >>P.y >> P.z >> P.lab ;
        }
        mes=0.;
        mesb=0.;
        
        if(nt == 0) {
                cerr << "  A meshS type must have elements  " << endl;
            ffassert(0);exit(1);
            
        }
        
        for (int k=0; k<nt; k++) {
            int i[3],lab;
            Element & K(this->elements[k]);
            f >> i[0] >> i[1] >> i[2] >> lab;
            Add(i,3,offset);
            K.set(this->vertices,i,lab);
            mes += K.mesure();
            err += K.mesure() <0;
                
        }
        
        
        
        for (int k=0; k<nbe; k++) {
            int i[2],lab;
            BorderElement & K(this->borderelements[k]);
            f >> i[0] >> i[1] >> lab;
            Add(i,2,offset);
            K.set(this->vertices,i,lab);
            mesb += K.mesure();
            
        }
        if(err!=0)
        {
            cerr << " MeshS::readmsh : sorry bad mesh. Number of negative Tri " << err << endl;
            this->~MeshS();
            ffassert(0);
        }
        
    }
    
    
    MeshS::MeshS(const string filename, double ridgeangledetection)
    :mapSurf2Vol(0),mapVol2Surf(0),meshL(0)  {
        int ok=load(filename);
        if(verbosity) {
            cout << "read meshS ok " << ok ;
            cout << "surface Mesh, num Triangles:= " << nt << ", num Vertice:= " << nv << " num boundary Edges:= " << nbe << endl;
        }
        
        if (ok) {
            ifstream f(filename.c_str());
            if(!f) {
                cerr << "  --  MeshS::MeshS Erreur opening " << filename<<endl;ffassert(0);exit(1);}
            if(verbosity>2)
                cout << "  -- MeshS:  Read On file \"" <<filename<<"\""<<  endl;
            if(filename.rfind(".msh")==filename.length()-4)
                readmsh(f,-1);
            else
                read(f);
        }
        
        BuildBound();
        BuildAdj();
        Buildbnormalv();
        BuildjElementConteningVertex();
        
        
        if(verbosity>2) cout << "  -- End of read: MeshS mesure = " << mes << " border mesure " << mesb << endl;
        
        if(verbosity) cout << "  -- MeshS : "<<filename  << ", space dimension "<< 3  << ", num Triangle elts " << nt << ", num Vertice "
            << nv << " num Bordary elts " << nbe << endl;
        
        
        ffassert(mes>=0); // add F. Hecht sep 2009.
        
    }
    
    
    
    
    int MeshS::load(const string & filename)
    {
        int bin;
        int ver,inm,dim;
        int lf=filename.size()+20;
        KN<char>  fileb(lf),filef(lf);
        char *data = new char[filename.size()+1];
        size_t ssize = filename.size()+1;
        char *ptr;
        char *pfile=data;
        strncpy( data, filename.c_str(),ssize);
        ptr = strstr(data,".mesh");
        if( !ptr ){
            strcpy(filef,filename.c_str());
            strcpy(fileb,filef);
            strcat(filef,".mesh");
            strcat(fileb,".meshb");
            if( (inm=GmfOpenMesh(pfile=fileb, GmfRead,&ver,&dim)) )
                bin=true;
            else if( (inm=GmfOpenMesh(pfile=filef, GmfRead,&ver,&dim)) )
                bin=false;
            else
                if(verbosity>5){
                    cerr << " Erreur ouverture file " << (char *) fileb  << " " << (char *) filef  <<endl;
                    return   1;
                }
        }
        else{
            if( !(inm=GmfOpenMesh(data, GmfRead,&ver,&dim)) ){
                if(verbosity>5)
                    cerr << " Erreur ouverture file " << (char *) data  << endl;
                return   1;
            }
        }
        // data file is readed and the meshes are initilized
        int nv=-1,nTet=-1,nTri=-1,nSeg=-1, nPt=-1;
        nv = GmfStatKwd(inm,GmfVertices);  // vertice
        nTri= GmfStatKwd(inm,GmfTriangles); // triangles in case of volume mesh -> boundary element / in case of surface mesh -> element
        nSeg=GmfStatKwd(inm,GmfEdges); // segment elements only present in surface mesh
        nPt=0; //GmfStatKwd(inm,GmfEdges); // points border on border mesh, not available at this time
        
        if (nTri>0 && nSeg>0 && nPt==0)
            if(verbosity>1)
                cout << "data file "<< pfile <<  " contains only a MeshS, the MeshL associated is created (whitout border points)." << endl;
        if (nTri>0 && nSeg>0 && nPt>0)
            if(verbosity>1) cout << "data file "<< pfile <<  " contains a MeshS and MeshL" << endl;
        if(verbosity && !nTri && !nTet)
            cerr << " WARNING!!! The mesh file just contains a set of vertices" << endl;

        
        
        this->set(nv,nTri,nSeg);
        nPoints=nPt;
        
        if(nTri == 0) {
            cerr << "  A meshS type must have elements  " << endl;
            ffassert(0);exit(1);}
        
        if(verbosity>1)
            cout << "  -- MeshS(load): "<< (char *) data <<  ", MeshVersionFormatted:= " << ver << ", space dimension:= "<< dim
            << ", num Triangles elts:= " << nTri << ", num vertice:= " << nv << " num Edges boundaries:= " << nSeg << endl;
        
        if(dim  != 3) {
            cerr << "Err dim == " << dim << " !=3 " <<endl;
            return 2; }
        if( nv<=0 && (nTri <=0 || nSeg <0) ) {
            cerr << " missing data "<< endl;
            return 3;
        }
        int iv[3],lab;
        float cr[3];
        int mxlab=0, mnlab=0;
        // read vertices
        GmfGotoKwd(inm,GmfVertices);
        for(int i=0;i<this->nv;++i) {
            if(ver<2) {
                GmfGetLin(inm,GmfVertices,&cr[0],&cr[1],&cr[2],&lab);
                vertices[i].x=cr[0];
                vertices[i].y=cr[1];
                vertices[i].z=cr[2];}
            else
                GmfGetLin(inm,GmfVertices,&vertices[i].x,&vertices[i].y,&vertices[i].z,&lab);
            vertices[i].lab=lab;
            mxlab= max(mxlab,lab);
            mnlab= min(mnlab,lab);
        }
        // read triangles (element meshS)
        if(mnlab==0 && mxlab==0 ) {
            int kmv=0;
            mes=0;
            GmfGotoKwd(inm,GmfTriangles);
            for(int i=0;i<nTri;++i) {
                GmfGetLin(inm,GmfTriangles,&iv[0],&iv[1],&iv[2],&lab);
                assert( iv[0]>0 && iv[0]<=nv && iv[1]>0 && iv[1]<=nv && iv[2]>0 && iv[2]<=nv);
                for(int j=0;j<3;++j)
                    if(!vertices[iv[j]-1].lab) {
                        vertices[iv[j]-1].lab=1;
                        kmv++;
                    }
                for (int j=0;j<3;++j) iv[j]--;
                elements[i].set(vertices,iv,lab);
                mes += elements[i].mesure();
            }
            if(kmv&& verbosity>1) cout << "    Aucun label Hack (FH)  ??? => 1 sur les triangle frontiere "<<endl;
        }
        else {
            mes=0;
            GmfGotoKwd(inm,GmfTriangles);
            for(int i=0;i<nTri;++i) {
                GmfGetLin(inm,GmfTriangles,&iv[0],&iv[1],&iv[2],&lab);
                assert( iv[0]>0 && iv[0]<=nv && iv[1]>0 && iv[1]<=nv && iv[2]>0 && iv[2]<=nv);
                for (int j=0;j<3;++j) iv[j]--;
                elements[i].set(this->vertices,iv,lab);
                mes += elements[i].mesure();
            }
        }
        // read edges (boundary elements meshS)
        mesb=0;
        GmfGotoKwd(inm,GmfEdges);
        for(int i=0;i<nSeg;++i) {
            GmfGetLin(inm,GmfEdges,&iv[0],&iv[1],&lab);
            assert( iv[0]>0 && iv[0]<=nv && iv[1]>0 && iv[1]<=nv);
            for (int j=0;j<2;j++) iv[j]--;
            borderelements[i].set(this->vertices,iv,lab);   //element
            mesb += this->borderelements[i].mesure();
        }
        
        // the .mesh contains edges, Building the meshS
        // for this, surface vertices must be extract of the original vertice list and a mapping must be created between surface and volume vertices
        if (nPoints>0) {
            meshL = new MeshL();
            // Number of Vertex in the surface
            meshL->mapSurf2Curv=new int[nv];
            meshL->mapCurv2Surf=new int[nv]; // mapping to curve/surface vertices
            for (int k=0; k<nv; k++) {
                meshL->mapSurf2Curv[k]=-1;
                meshL->mapCurv2Surf[k]=0;
            }
            // search Vertex on the surface
            int nbv_curv=0;
            for (int k=0; k<nbe; k++) {
                const BorderElement & K(this->borderelements[k]);
                for(int jj=0; jj<2; jj++) {
                    int i0=this->operator()(K[jj]);
                    if( meshL->mapSurf2Curv[i0] == -1 ) {
                        // the mapping v_num_surf -> new numbering /  liste_v_num_surf[nbv_surf] -> the global num
                        meshL->mapSurf2Curv[i0] = nbv_curv;
                        meshL->mapCurv2Surf[nbv_curv]= i0;
                        nbv_curv++;
                    }
                }
            }
            this->meshL->set(nbv_curv,nSeg,nPoints);
            // save the surface vertices
            for (int k=0; k<nbv_curv; k++) {
                int k0 = meshL->mapCurv2Surf[k];
                const  Vertex & P = this->vertices[k0];
                meshL->vertices[k].lab=P.lab;
                meshL->vertices[k].x=P.x;
                meshL->vertices[k].y=P.y;
                meshL->vertices[k].z=P.z;
            }
            // read triangles and change with the surface numbering
            int kmv=0;
            meshL->mes=0;
            GmfGotoKwd(inm,GmfEdges);
            for(int i=0;i<nSeg;++i) {
                GmfGetLin(inm,GmfEdges,&iv[0],&iv[1],&lab);
                for (int j=0;j<2;++j)
                    iv[j]=meshL->mapSurf2Curv[iv[j]-1];
                for(int j=0;j<2;++j)
                    if(!meshL->vertices[iv[j]].lab) {
                        meshL->vertices[iv[j]].lab=1;
                        kmv++;
                    }
                meshL->elements[i].set(meshL->vertices,iv,lab);
                meshL->mes += meshL->elements[i].mesure();
            }
            // reading border points with the curv numbering  not available at this moment
            meshL->mesb=0;
            /*GmfGotoKwd(inm,GmfEdges);
            for(int i=0;i<nSeg;++i) {
                GmfGetLin(inm,GmfEdges,&iv[0],&iv[1],&lab);
                for (int j=0;j<2;++j) iv[j]=meshS->mapVol2Surf[iv[j]-1];
                // assert( iv[0]>0 && iv[0]<=nbv_surf && iv[1]>0 && iv[1]<=nbv_surf);
                meshS->borderelements[i].set(meshS->vertices,iv,lab);
                meshS->mesb += meshS->borderelements[i].mesure();
            }*/
        }
        else
            if(verbosity>5) cout << " use Th = buildBdMesh(Th) to build the curve mesh associated" << endl;

        
        if(verbosity>1)
            cout << "  -- MeshS(load): "<< (char *) data <<  ", MeshVersionFormatted:= " << ver << ", space dimension:= "<< dim
            << ", Triangle elts:= " << nt << ", num vertice:= " << nv << ", num edges boundaries:= " << nbe << endl;
        if(verbosity>1)
            cout << "  -- MeshS::MeshL(load): "<< (char *) data <<  ", MeshVersionFormatted:= " << ver << ", space dimension:= "<< dim
            << ", Edges elts:= " << meshL->nt << ", num vertice:= " << meshL->nv << ", num border points:= " << meshL->nbe << endl;
        
        GmfCloseMesh(inm);
        delete[] data;
        return 0; // OK
    }
    
    
    MeshS::MeshS(const string filename, bool cleanmesh, bool removeduplicate, bool rebuildboundary, int orientation, double precis_mesh, double ridgeangledetection)
    :mapSurf2Vol(0),mapVol2Surf(0),meshL(0) {
        
        
        int ok=load(filename);
        if(verbosity) {
            cout << "read mesh ok " << ok  << endl;
            cout << ", nt " << nt << ", nv " << nv << " nbe:  = " << nbe << endl;
        }
        if(ok)
        {
            ifstream f(filename.c_str());
            if(!f) {
                cerr << "  --  MeshS: Erreur opening " << filename<<endl;ffassert(0);exit(1);}
            if(verbosity>2)
                cout << "  -- MeshS:  Read On file \"" <<filename<<"\""<<  endl;
            if(filename.rfind(".msh")==filename.length()-4)
                readmsh(f,-1);
            else
                read(f);
        }
        if (cleanmesh) {
            if(verbosity>3)
                cout << "before clean meshS, nv: " <<nv << " nt:" << nt << " nbe:" << nbe << endl;
            clean_mesh(precis_mesh, nv, nt, nbe, vertices, elements, borderelements, removeduplicate, rebuildboundary, orientation);
            if(verbosity>3)
                cout << "after clean meshS, nv: " <<nv << " nt:" << nt << " nbe:" << nbe << endl;
        }
        
        BuildBound();
        BuildAdj();
        Buildbnormalv();
        BuildjElementConteningVertex();
        
        // if not edges then build the edges - need access to the old adjacensce to build eges and rebuild the new adj
        if (nbe==0) {
            BuildEdges();
            delete [] TheAdjacencesLink;
            delete [] BoundaryElementHeadLink;
            TheAdjacencesLink=0;
            BoundaryElementHeadLink=0;
            BuildBound();
            BuildAdj();
            Buildbnormalv();
            BuildjElementConteningVertex();
        }
        else BuildMeshL(ridgeangledetection);
        
        if(verbosity>2)
            cout << "  -- End of read: mesure = " << mes << " border mesure " << mesb << endl;
        if(verbosity)
            cout << "  -- MeshS : "<<filename  << ", d "<< 3  << ", n Tri " << nt << ", n Vtx "
            << nv << " n Border Edges " << nbe << endl;
        ffassert(mes>=0); // add F. Hecht sep 2009.
    }
    
 
    
    MeshS::MeshS(FILE *f,int offset)
    :mapSurf2Vol(0),mapVol2Surf(0),meshL(0)
    {
        GRead(f,offset);// remove 1
        assert( (nt >= 0 || nbe>=0)  && nv>0) ;
        BuildBound();
        if(verbosity>2)
            cout << "  -- End of read: mesure = " << mes << " border mesure " << mesb << endl;
      
        BuildAdj();
        Buildbnormalv();
        BuildjElementConteningVertex();
        
        // if not edges then build the edges - need access to the old adjacensce to build eges and rebuild the new adj
        if (nbe==0) {
            BuildEdges();
            delete [] TheAdjacencesLink;
            delete [] BoundaryElementHeadLink;
            TheAdjacencesLink=0;
            BoundaryElementHeadLink=0;
            BuildBound();
            BuildAdj();
            Buildbnormalv();
            BuildjElementConteningVertex();
        }
        
        if(verbosity>2)
            cout << "  -- End of read: mesure = " << mes << " border mesure " << mesb << endl;
        
        if(verbosity>1)
            cout << "  -- MeshS  (File *), d "<< 3  << ", n Tri " << nt << ", n Vtx "
            << nv << " n Bord " << nbe << endl;
        ffassert(mes>=0); // add F. Hecht sep 2009.
    }
    
    
    double MeshS::hmin() const
    {
        R3 Pinf(1e100,1e100,1e100),Psup(-1e100,-1e100,-1e100);   // Extremite de la boite englobante
        double hmin=1e10;
        
        for (int ii=0;ii< this->nv;ii++) {
            R3 P( vertices[ii].x, vertices[ii].y, vertices[ii].z);
            Pinf=Minc(P,Pinf);
            Psup=Maxc(P,Psup);
        }
        
        for (int k=0;k<this->nt;k++) {
            for (int e=0;e<3;e++){
                if( this->elements[k].lenEdge(e) < Norme2(Psup-Pinf)/1e9 ){
                    const TriangleS & K(this->elements[k]);
                    int iv[3];
                    for(int jj=0; jj <3; jj++)
                        iv[jj] = this->operator()(K[jj]);
                    if(verbosity>2) for (int eh=0;eh<3;eh++)
                        cout << "TriangleS: " << k << " edge : " << eh << " lenght "<<  this->elements[k].lenEdge(eh) << endl;
                    if(verbosity>2) cout << " A triangleS with a very small edge was created " << endl;
                    return 1;
                }
                hmin=min(hmin,this->elements[k].lenEdge(e));   // calcul de .lenEdge pour un Mesh3
            }
        }
        ffassert(hmin>Norme2(Psup-Pinf)/1e9);
        return hmin;
    }
    
    
    // brute force method
    void MeshS::GRead(FILE * ff,int offset)
    {
        PlotStream f(ff);
        string s;
        f >> s;
        ffassert( s== GsbeginS);
        f >> nv >> nt >> nbe;
        if(verbosity>2)
            cout << " GRead : nv " << nv << " " << nt << " " << nbe << endl;
        this->vertices = new Vertex[nv];
        this->elements = new Element [nt];
        this->borderelements = new BorderElement[nbe];
        for (int k=0; k<nv; k++) {
            Vertex & P = this->vertices[k];
            f >> P.x >>P.y >> P.z >> P.lab ;
        }
        mes=0.;
        mesb=0.;
        
        if(nt == 0) {
            cerr << "  A meshS type must have elements  " << endl;
            ffassert(0);exit(1);}
 
            
        for (int k=0; k<nt; k++) {
            int i[3],lab;
            Element & K(this->elements[k]);
            f >> i[0] >> i[1] >> i[2]  >> lab;
            Add(i,3,offset);
            K.set(this->vertices,i,lab);
            mes += K.mesure();
    
        }
        for (int k=0; k<nbe; k++) {
            int i[2],lab;
            BorderElement & K(this->borderelements[k]);
            f >> i[0] >> i[1] >> lab;
            Add(i,2,offset);
            K.set(this->vertices,i,lab);
            mesb += K.mesure();
            
        }
        f >> s;
        ffassert( s== GsendS);
    }
    
    
    const MeshS::Element * MeshS::Find( Rd P, R2 & Phat,bool & outside,const Element * tstart) const //;
    
    {
        
        for (int i=0;i<nt;i++)
        {
            kthrough++;
            const TriangleS & K(this->elements[i]);
            R3 A(K[0]),B(K[1]),C(K[2]);
            // the normal n
            R3 n = (B-A)^(C-A);
            // to build the vectorial area
            R a=(Area3(P,B,C),n);
            R b=(Area3(A,P,C),n);
            R c=(Area3(A,B,P),n);
            R s=a+b+c;
            R eps=s*1e-6;
            if (a>-eps && b >-eps && c >-eps && abs((P-A,n)) < eps) {
                Phat=R2(b/s,c/s);
                return this->elements + i;
            }
        }
        return 0; // outside
    }
    
    
    
    MeshS::MeshS(int nnv, int nnt, int nnbe, Vertex3 *vv, TriangleS *tt, BoundaryEdgeS *bb, bool cleanmesh, bool removeduplicate, bool rebuildboundary, int orientation, double precis_mesh)
    :mapVol2Surf(0),mapSurf2Vol(0),meshL(0)
    {
        nv = nnv;
        nt = nnt;
        nbe =nnbe;
        vertices = vv;
        elements = tt;
        borderelements = bb;
        mes=0.;
        mesb=0.;
        
        for (int i=0;i<nt;i++)
            mes += this->elements[i].mesure();
        for (int i=0;i<nbe;i++)
            mesb += this->be(i).mesure();

         if (cleanmesh) {
            if(verbosity>5)
                cout << "before clean meshS, nv: " <<nv << " nt:" << nt << " nbe:" << nbe << endl;
            clean_mesh(precis_mesh, nv, nt, nbe, vertices, elements, borderelements, removeduplicate, rebuildboundary, orientation);
            if(verbosity>5)
                cout << "after clean meshS, nv: " <<nv << " nt:" << nt << " nbe:" << nbe << endl;
        }
        BuildBound();
        BuildAdj();
        Buildbnormalv();
        BuildjElementConteningVertex();
        // if not edges then build the edges - need access to the old adjacensce to build eges and rebuild the new adj
        if (nbe==0) {
            if(verbosity>3)
                cout << " building of boundary " << endl;
            BuildEdges();
            delete [] TheAdjacencesLink;
            delete [] BoundaryElementHeadLink;
            TheAdjacencesLink=0;
            BoundaryElementHeadLink=0;
            BuildBound();
            BuildAdj();
            Buildbnormalv();
            BuildjElementConteningVertex();
        }
        
        
        if(verbosity>1)
            cout << "  -- End of read meshS: mesure = " << mes << " border mesure " << mesb << endl;
        
        assert(mes>=0.);
    }
   
    
    MeshS::MeshS(const  Serialize &serialized)
    :GenericMesh<TriangleS,BoundaryEdgeS,Vertex3> (serialized),
    mapVol2Surf(0), mapSurf2Vol(0), meshL(0)
    {
        BuildBound();
        if(verbosity>1)
            cout << "  -- End of serialized: mesure = " << mes << " border mesure " << mesb << endl;
        

            BuildAdj();
            Buildbnormalv();
            BuildjElementConteningVertex();

        
        if(verbosity>1)
            cout << "  -- MeshS  (serialized), d "<< 3  << ", n Vtx " << nv <<", n Tri " << nt <<  " n Bord " << nbe << endl;
        ffassert(mes>=0); // add F. Hecht sep 2009.
        
    }
    
    
    int MeshS::Save(const string & filename) const
    {
        int ver = GmfDouble, outm;
        if ( !(outm = GmfOpenMesh(filename.c_str(),GmfWrite,ver,3)) ) {
            cerr <<"  -- MeshS**::Save  UNABLE TO OPEN  :"<< filename << endl;
            return(1);
        }
        float fx,fy,fz;
        // write vertice (meshS)
        GmfSetKwd(outm,GmfVertices,nv);
        for (int k=0; k<nv; k++) {
            const  Vertex & P = vertices[k];
            GmfSetLin(outm,GmfVertices,fx=P.x,fy=P.y,fz=P.z,P.lab);
        }
        // write triangles (meshS)
        GmfSetKwd(outm,GmfTriangles,nt);
        for (int k=0; k<nt; k++) {
            const MeshS::Element & K(elements[k]);
            int i0=this->operator()(K[0])+1;
            int i1=this->operator()(K[1])+1;
            int i2=this->operator()(K[2])+1;
            int lab=K.lab;
            GmfSetLin(outm,GmfTriangles,i0,i1,i2,lab);
        }
        // write edges (meshS)
        GmfSetKwd(outm,GmfEdges,nbe);
        for (int k=0; k<nbe; k++) {
            const BorderElement & K(borderelements[k]);
            int i0=this->operator()(K[0])+1;
            int i1=this->operator()(K[1])+1;
            int lab=K.lab;
            GmfSetLin(outm,GmfEdges,i0,i1,lab);
        }
        GmfCloseMesh(outm);
        return (0);
    }
    
    
    Serialize MeshS::serialize_withBorderMesh() const {
        
        // structure here for MeshL...but not use now
        ffassert(0);
    }
    
    
    
    
    
    void MeshS::flipSurfaceMeshS(int surface_orientation)
    {
        /* inverse the orientation of the surface if necessary*/
        /* and control that all surfaces are oriented in the same way*/
        int nbflip=0;
        for (int i=0;i<this->nt;i++) {
            double mes_triangleS= this->elements[i].mesure();
            
            if( surface_orientation*mes_triangleS < 0){
                const TriangleS &K(elements[i] );
                int iv[3];
                
                for (int j=0 ; j<3 ; j++)
                    iv[j] = this->operator()(K[j]);
                
                int iv_temp=iv[1];
                iv[1]=iv[2];
                iv[2]=iv_temp;
                this->elements[i].set( this->vertices, iv, K.lab ) ;
                nbflip++;
            }
        }
        assert(nbflip==0 || nbflip== this->nt);
    }
    
    
    // determine the boundary edge list for meshS
    void MeshS::BuildEdges(const double angle) {
        
        delete [] borderelements; // to remove the previous pointers
        borderelements = new BoundaryEdgeS[3 * nt]; // 3 * nt upper evaluated
        
        HashTable<SortArray<int, 2>, int> edgesI(3 * nt, nt);
        int* AdjLink = new int[3 * nt];
        
        int nbeS=0,nbiS=0,nk=0;
        // Build edges from the triangle list
        for (int i = 0; i < nt; i++)
            for (int j = 0; j < 3; j++) {
                
                int jt = j, it = ElementAdj(i, jt);
                TriangleS &K(elements[i]);  // current element
                
                // True boundary edge -> no adjacence / on domain border
                if ((it == i || it < 0)) {
                    int iv[2];
                    for(int ip=0;ip<2;ip++)
                        iv[ip] = this->operator () (K [TriangleS::nvedge[j][ip]]);
                    if(verbosity>15)
                        cout << " the edge " << iv[0] << " " << iv[1] << " is a boundary " << endl;
                    be(nbeS).set(vertices,iv,K.lab);
                    mesb += be(nbeS).mesure();
                    nbeS++;
                    
                }
                // internal edge -- check angular and no manifold
                else {
                    TriangleS &K_adj(elements[it]); //adjacence element
                    int iv[2];
                    for(int ip=0;ip<2;ip++)
                        iv[ip] = this->operator () (K [TriangleS::nvedge[j][ip]]);
                    
                    SortArray<int, 2> key(iv[0], iv[1]);
                    typename HashTable<SortArray<int,2>,int>::iterator p= edgesI.find(key);
                    if (!p) {    // 1st time the edge is seen
                        // unit normal
                        R3 Normal = K.NormalSUnitaire();
                        R3 Normal_adj = K_adj.NormalS();
                        Normal_adj /= Normal_adj.norme();
                        R pdt = (Normal,Normal_adj); // scalar product
                        pdt = acos(pdt); // radian angle (Normal,Normal_adj)
                        if(verbosity>15)
                            cout << "Element num: " << i << " N " << Normal << " Element adjacent num: " << it << " N_adj " << Normal_adj << " angle between N N_adj = " << pdt <<endl;
                        
                        if(pdt >= angle) {
                            if(verbosity>15)
                                cout << " the edge " <<nbeS <<": [" << iv[0] << " " << iv[1] << "] is a boundary with the angular criteria" << endl;
                            int lab = min(K.lab, K_adj.lab);
                            //cout << "oldnum" <<liste_v_num_surf[iv[0]] <<" /" << liste_v_num_surf[iv[1]] << endl;

                            be(nbeS).set(vertices,iv,lab);
                            mesb += be(nbeS).mesure();
                            edgesI.add(key, nbeS++);
                        }
                    }
                    // the edge is internal --- manifold or no?
                    else {
                        // the edge is adjacent to more 2 triangles -> no manifold -> it's a boundary edge
                        if (p->v < 0) {
                            int nk1=-1-p->v;
                            int nk2= AdjLink[nk1];
                            int lab = min(K.lab, K_adj.lab);
                            be(nbeS).set(vertices,iv,lab);
                            mesb += be(nbeS).mesure();
                            nbeS++;
                            if(nk2>=0) { // firt time remove existing link ...
                                AdjLink[nk1]=-2;
                                AdjLink[nk2]=-2;// on no manifold border .
                            }
                        }
                        else {
                            AdjLink[nk]=p->v;
                            AdjLink[p->v]=nk;
                            p->v=-1-nk;
                            nbiS++;
                        }
                    }
                }
                nk++;  // increment the total edge jump --- nt * 3
                
            }
        assert(nt*3==nk);
        delete [] AdjLink;
        // update the number of edges
        nbe = nbeS;
        if (verbosity>5) cout << " Building edges from mesh3 nbe: "<< nbeS << " nbi: " << nbiS << endl;
        
        BuildBound();
        delete []TheAdjacencesLink;
        delete [] BoundaryElementHeadLink;
        TheAdjacencesLink=0;
        BoundaryElementHeadLink=0;
        BuildAdj();
        Buildbnormalv();
        BuildjElementConteningVertex();
    }
    
    
    void MeshS::BuildMeshL(double angle)
    {
        
        if (meshL) {
            cout << "error, MeshS::meshL previously created " << endl;
            return;
        }
        if (verbosity) cout << "Build meshL from meshS.... " << endl;
        
        
        int mes = 0, mesb = 0;
        
        int *v_num_curve, *map_v_num_curve;
        // Extraction of Vertex  belongs to the surface
        v_num_curve=new int[nv];
        map_v_num_curve=new int[nv];
        for (int k=0; k<nv; k++){
            v_num_curve[k]=-1;
            map_v_num_curve[k]=0;
        }
        // Search Vertex on the surface
        int nbv_curve=0;
        for (int k=0; k<nbe; k++) {
            const BoundaryEdgeS & K(borderelements[k]);
            for(int jj=0; jj<2; jj++){
                int i0=this->operator()(K[jj]);
                if( v_num_curve[i0] == -1 ){
                    v_num_curve[i0] = nbv_curve;
                    map_v_num_curve[nbv_curve]= i0;
                    nbv_curve++;
                }
            }
        }
        
        // set the curve vertex in meshL
        ffassert(nbv_curve);
        
        Vertex3 *vL = new Vertex3[nbv_curve];
        EdgeL *tL = new EdgeL[nbe];
        EdgeL *ttL = tL;
        
        
        for (int iv=0; iv<nbv_curve; iv++) {
            int k0 = map_v_num_curve[iv];
            const Vertex3 & P = vertices[k0];
            vL[iv].x=P.x;
            vL[iv].y=P.y;
            vL[iv].z=P.z;
            vL[iv].lab=P.lab;
        }
        
        ffassert(nbe);
        
        
        for (int it=0; it<nbe; it++) {
            int iv[2];
            const BoundaryEdgeS & K(borderelements[it]);
            for (int j=0;j<2;++j)
                iv[j]=v_num_curve[this->operator()(K[j])];
             int lab=K.lab;
            (ttL)->set(vL,iv,lab);
            mes += ttL++->mesure();
        }
        
        // first building without list edges
        
        meshL = new MeshL(nbv_curve,nbe,0,vL,tL,0);
        meshL->mapSurf2Curv = new int[nv];
        meshL->mapCurv2Surf= new int[nv];
        for(int i=0 ; i<nv ; i++) {
            meshL->mapSurf2Curv[i] = v_num_curve[i];
            meshL->mapCurv2Surf[i] = map_v_num_curve[i];
        }
        meshL->BuildBorderPt(angle);
        meshL->BuildGTree();
        delete [] v_num_curve;
        delete [] map_v_num_curve;
        
    }
}
