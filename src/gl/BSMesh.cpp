#include "BSMesh.h"
#include "message.h"
#include "gl/controllers.h"
#include "gl/glscene.h"
#include "gl/renderer.h"
#include "io/material.h"
#include "io/nifstream.h"
#include "model/nifmodel.h"

#include <QDir>
#include <QBuffer>


BSMesh::BSMesh(Scene* s, const QModelIndex& iBlock) : Shape(s, iBlock)
{
}

void BSMesh::transformShapes()
{
	// TODO: implement this
#if 0
	if ( isHidden() )
		return;
#endif
}

void BSMesh::drawShapes( NodeList * secondPass )
{
	if ( isHidden() || ( !scene->hasOption(Scene::ShowMarkers) && name.contains("EditorMarker") ) )
		return;

	// Draw translucent meshes in second pass
	if ( secondPass && drawInSecondPass ) {
		secondPass->add(this);
		return;
	}

	auto nif = NifModel::fromIndex(iBlock);
	if ( lodLevel != scene->lodLevel ) {
		lodLevel = scene->lodLevel;
		updateData(nif);
	}

	glPushMatrix();
	glMultMatrix(viewTrans());

	glEnable(GL_POLYGON_OFFSET_FILL);
	if ( drawInSecondPass )
		glPolygonOffset(0.5f, 1.0f);
	else
		glPolygonOffset(1.0f, 2.0f);


	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, transVerts.constData());

	if ( Node::SELECTING ) {
		if ( scene->isSelModeObject() ) {
			int s_nodeId = ID2COLORKEY(nodeId);
			glColor4ubv((GLubyte*)&s_nodeId);
		} else {
			glColor4f(0, 0, 0, 1);
		}
	}

	if ( !Node::SELECTING ) {
		glEnable(GL_FRAMEBUFFER_SRGB);
		shader = scene->renderer->setupProgram(this, shader);

	} else {
		glDisable(GL_FRAMEBUFFER_SRGB);
	}

	if ( !Node::SELECTING ) {
		if ( transNorms.count() ) {
			glEnableClientState(GL_NORMAL_ARRAY);
			glNormalPointer(GL_FLOAT, 0, transNorms.constData());
		}

		if ( transColors.count() && scene->hasOption(Scene::DoVertexColors) ) {
			glEnableClientState(GL_COLOR_ARRAY);
			glColorPointer(4, GL_FLOAT, 0, transColors.constData());
		} else {
			glColor(Color3(1.0f, 1.0f, 1.0f));
		}
	}

	if ( sortedTriangles.count() )
		glDrawElements(GL_TRIANGLES, sortedTriangles.count() * 3, GL_UNSIGNED_SHORT, sortedTriangles.constData());

	if ( !Node::SELECTING )
		scene->renderer->stopProgram();

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glDisable(GL_POLYGON_OFFSET_FILL);

	glPopMatrix();
}

void BSMesh::drawSelection() const
{
	if ( scene->hasOption(Scene::ShowNodes) )
		Node::drawSelection();

	if ( isHidden() || !scene->isSelModeObject() )
		return;

	auto& blk = scene->currentBlock;
	auto& idx = scene->currentIndex;
	auto nif = NifModel::fromValidIndex(blk);

	glDisable(GL_LIGHTING);
	glDisable(GL_COLOR_MATERIAL);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_NORMALIZE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_CULL_FACE);

	glDisable(GL_FRAMEBUFFER_SRGB);
	glPushMatrix();
	glMultMatrix(viewTrans());

	if ( blk == iBlock ) {

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		glEnable(GL_POLYGON_OFFSET_FILL);
		glPolygonOffset(-1.0f, -2.0f);

		glPointSize(1.5f);
		glLineWidth(1.6f);
		glNormalColor();

		// Name of this index
		auto n = idx.data( NifSkopeDisplayRole ).toString();

		if ( n == "Bounding Sphere" ) {
			auto sph = BoundSphere( nif, idx );
			if ( sph.radius > 0.0f ) {
				glColor4f( 1, 1, 1, 0.33f );
				drawSphereSimple( sph.center, sph.radius, 72 );
			}
		} else if ( n == "Bound Min Max" ) {
			Vector3	boundsDims( nif->get<float>( idx, 3 ), nif->get<float>( idx, 4 ), nif->get<float>( idx, 5 ) );
			if ( boundsDims[0] > 0.0f && boundsDims[1] > 0.0f && boundsDims[2] > 0.0f ) {
				Vector3	boundsCenter( nif->get<float>( idx, 0 ), nif->get<float>( idx, 1 ), nif->get<float>( idx, 2 ) );
				glColor4f( 1, 1, 1, 0.33f );
				drawBox( boundsCenter - boundsDims, boundsCenter + boundsDims );
			}
		} else {
			for ( const Triangle& tri : sortedTriangles ) {
				glBegin(GL_TRIANGLES);
				glVertex(transVerts.value(tri.v1()));
				glVertex(transVerts.value(tri.v2()));
				glVertex(transVerts.value(tri.v3()));
				glEnd();
			}
		}

		glDisable(GL_POLYGON_OFFSET_FILL);

#if 0 && !defined(QT_NO_DEBUG)
		drawSphereSimple(boundSphere.center, boundSphere.radius, 72);
#endif
	}

	glPopMatrix();
}

BoundSphere BSMesh::bounds() const
{
	if ( needUpdateBounds ) {
		needUpdateBounds = false;
		if ( transVerts.count() ) {
			boundSphere = BoundSphere(transVerts);
		} else {
			boundSphere = dataBound;
		}
	}

	return worldTrans() * boundSphere;
}

QString BSMesh::textStats() const
{
	return QString();
}

void BSMesh::forMeshIndex(const NifModel* nif, std::function<void(const QString&, int)>& f)
{
	for ( int i = 0; i < 4; i++ ) {
		auto meshArray = QModelIndex_child( iMeshes, i );
		bool hasMesh = nif->get<bool>( QModelIndex_child( meshArray ) );
		auto mesh = QModelIndex_child( meshArray, 1 );
		if ( hasMesh ) {
			auto meshPath = nif->get<QString>(mesh, "Mesh Path");
			if ( !meshPath.startsWith("geometries", Qt::CaseInsensitive) ) {
				meshPath = "geometries\\" + meshPath;
			}
			if ( !meshPath.endsWith(".mesh") ) {
				meshPath += ".mesh";
			}
			f(meshPath, i);
		}
	}
}

int BSMesh::meshCount()
{
	return meshes.size();
}

void BSMesh::drawVerts() const
{
	return;
	glDisable(GL_LIGHTING);
	glNormalColor();

	glBegin(GL_POINTS);
	for ( int i = 0; i < transVerts.count(); i++ ) {
		if ( Node::SELECTING ) {
			int id = ID2COLORKEY((shapeNumber << 16) + i);
			glColor4ubv((GLubyte*)&id);
		}
		glVertex(transVerts.value(i));
	}
	glEnd();
}

QModelIndex BSMesh::vertexAt(int) const
{
	return QModelIndex();
}

void BSMesh::updateImpl(const NifModel* nif, const QModelIndex& index)
{
	qDebug() << "updateImpl";
	Shape::updateImpl(nif, index);
	if ( index != iBlock )
		return;

	iData = index;
	iMeshes = nif->getIndex(index, "Meshes");
	meshes.clear();
	std::function<void(const QString&, int)> createMeshFile = [&](const QString& meshPath, int lodLevel) {
		auto mesh = std::make_shared<MeshFile>(meshPath);
		if ( mesh->isValid() ) {
			meshes.append(mesh);
			if ( lodLevel > 0 || mesh->lods.size() > 0 )
				emit nif->lodSliderChanged(true);
		}
	};

	forMeshIndex(nif, createMeshFile);
}

void BSMesh::updateData(const NifModel* nif)
{
	qDebug() << "updateData";
	resetSkinning();
	resetVertexData();
	resetSkeletonData();
	gpuLODs.clear();
	boneNames.clear();
	boneTransforms.clear();

	if ( meshes.size() == 0 )
		return;

	bool hasMeshLODs = meshes[0]->lods.size() > 0;
	int lodCount = (hasMeshLODs) ? meshes[0]->lods.size() + 1 : meshes.size();

	if ( hasMeshLODs && meshes.size() > 1 ) {
		qWarning() << "Both static and skeletal mesh LODs exist";
	}

	lodLevel = std::min(scene->lodLevel, Scene::LodLevel(lodCount - 1));

	auto meshIndex = (hasMeshLODs) ? 0 : lodLevel;
	if ( lodCount > int(lodLevel) ) {
		auto& mesh = meshes[meshIndex];
		if ( lodLevel > 0 && int(lodLevel) <= mesh->lods.size() ) {
			sortedTriangles = mesh->lods[lodLevel - 1];
		}
		else {
			sortedTriangles = mesh->triangles;
		}
		transVerts = mesh->positions;
		coords.resize( mesh->haveTexCoord2 ? 2 : 1 );
		coords[0].resize( mesh->coords.size() );
		for ( int i = 0; i < mesh->coords.size(); i++ ) {
			coords[0][i][0] = mesh->coords[i][0];
			coords[0][i][1] = mesh->coords[i][1];
		}
		if ( mesh->haveTexCoord2 ) {
			coords[1].resize( mesh->coords.size() );
			for ( int i = 0; i < mesh->coords.size(); i++ ) {
				coords[1][i][0] = mesh->coords[i][2];
				coords[1][i][1] = mesh->coords[i][3];
			}
		}
		transColors = mesh->colors;
		hasVertexColors = !transColors.empty();
		transNorms = mesh->normals;
		transTangents = mesh->bitangents;
		transBitangents = mesh->tangents;
		weightsUNORM = mesh->weights;
		gpuLODs = mesh->lods;

		boundSphere = BoundSphere(transVerts);
		boundSphere.applyInv(viewTrans());
	}

	auto links = nif->getChildLinks(nif->getBlockNumber(iBlock));
	for ( const auto link : links ) {
		auto idx = nif->getBlockIndex(link);
		if ( nif->blockInherits(idx, "BSShaderProperty") ) {
			materialPath = nif->get<QString>(idx, "Name");
		} else if ( nif->blockInherits(idx, "NiIntegerExtraData") ) {
			materialID = nif->get<int>(idx, "Integer Data");
		} else if ( nif->blockInherits(idx, "BSSkin::Instance") ) {
			iSkin = idx;
			iSkinData = nif->getBlockIndex(nif->getLink(nif->getIndex(idx, "Data")));
			skinID = nif->getBlockNumber(iSkin);

			auto iBones = nif->getLinkArray(iSkin, "Bones");
			for ( const auto b : iBones ) {
				if ( b == -1 )
					continue;
				auto iBone = nif->getBlockIndex(b);
				boneNames.append(nif->resolveString(iBone, "Name"));
			}

			auto numBones = nif->get<int>(iSkinData, "Num Bones");
			boneTransforms.resize(numBones);
			auto iBoneList = nif->getIndex(iSkinData, "Bone List");
			for ( int i = 0; i < numBones; i++ ) {
				auto iBone = QModelIndex_child( iBoneList, i );
				Transform trans;
				trans.rotation = nif->get<Matrix>(iBone, "Rotation");
				trans.translation = nif->get<Vector3>(iBone, "Translation");
				trans.scale = nif->get<float>(iBone, "Scale");
				boneTransforms[i] = trans;
			}
		}
	}
	// Do after dependent blocks above
	for ( const auto link : links ) {
		auto idx = nif->getBlockIndex(link);
		if ( nif->blockInherits(idx, "SkinAttach") ) {
			boneNames = nif->getArray<QString>(idx, "Bones");
			if ( std::all_of(boneNames.begin(), boneNames.end(), [](const QString& name) { return name.isEmpty(); }) ) {
				boneNames.clear();
				auto iBones = nif->getLinkArray(nif->getIndex(iSkin, "Bones"));
				for ( const auto& b : iBones ) {
					auto iBone = nif->getBlockIndex(b);
					boneNames.append(nif->resolveString(iBone, "Name"));
				}
			}
		}
	}
}
