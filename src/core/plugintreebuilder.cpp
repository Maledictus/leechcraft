/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#include "plugintreebuilder.h"
#include <boost/graph/visitors.hpp>
#include <boost/graph/depth_first_search.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/topological_sort.hpp>
#include <util/sll/containerconversions.h>
#include "interfaces/iinfo.h"
#include "interfaces/iplugin2.h"
#include "interfaces/ipluginready.h"

namespace LC
{
	PluginTreeBuilder::VertexInfo::VertexInfo ()
	: IsFulfilled_ (false)
	, Object_ (0)
	{
	}

	PluginTreeBuilder::VertexInfo::VertexInfo (QObject *obj)
	: IsFulfilled_ (false)
	, Object_ (obj)
	{
		IInfo *info = qobject_cast<IInfo*> (obj);
		if (!info)
		{
			qWarning () << Q_FUNC_INFO
					<< obj
					<< "doesn't implement IInfo";
			throw std::runtime_error ("VertexInfo creation failed.");
		}

		AllFeatureDeps_ = Util::AsSet (info->Needs ());
		UnfulfilledFeatureDeps_ = AllFeatureDeps_;
		FeatureProvides_ = Util::AsSet (info->Provides ());

		const auto ipr = qobject_cast<IPluginReady*> (obj);
		if (ipr)
			P2PProvides_ = ipr->GetExpectedPluginClasses ();

		const auto ip2 = qobject_cast<IPlugin2*> (obj);
		if (ip2)
		{
			AllP2PDeps_ = ip2->GetPluginClasses ();
			UnfulfilledP2PDeps_ = AllP2PDeps_;
		}
	}

	PluginTreeBuilder::PluginTreeBuilder ()
	{
	}

	void PluginTreeBuilder::AddObjects (const QObjectList& objs)
	{
		Instances_ << objs;
	}

	void PluginTreeBuilder::RemoveObject (QObject *obj)
	{
		Instances_.removeAll (obj);
	}

	template<typename Edge>
	struct CycleDetector : public boost::default_dfs_visitor
	{
		QList<Edge>& BackEdges_;

		CycleDetector (QList<Edge>& be)
		: BackEdges_ (be)
		{
		}

		template<typename Graph>
		void back_edge (Edge edge, Graph&)
		{
			BackEdges_ << edge;
		}
	};

	template<typename Graph, typename Vertex>
	struct FulfillableChecker : public boost::default_dfs_visitor
	{
		Graph& G_;
		const QList<Vertex>& BackVerts_;
		const QMap<Vertex, QList<Vertex>>& Reachable_;

		FulfillableChecker (Graph& g,
				const QList<Vertex>& backVerts,
				const QMap<Vertex, QList<Vertex>>& reachable)
		: G_ (g)
		, BackVerts_ (backVerts)
		, Reachable_ (reachable)
		{
		}

		void finish_vertex (Vertex u, const Graph&)
		{
			if (BackVerts_.contains (u))
			{
				qWarning () << G_ [u].Object_ << "is backedge";
				return;
			}

			G_ [u].IsFulfilled_ = G_ [u].UnfulfilledFeatureDeps_.isEmpty () &&
					G_ [u].UnfulfilledP2PDeps_.isEmpty ();
			if (!G_ [u].IsFulfilled_)
				qWarning () << qobject_cast<IInfo*> (G_ [u].Object_)->GetName ()
						<< "failed to initialize because of:"
						<< G_ [u].UnfulfilledFeatureDeps_
						<< G_ [u].UnfulfilledP2PDeps_;

			for (const auto& v : Reachable_ [u])
				if (!G_ [v].IsFulfilled_)
				{
					G_ [u].IsFulfilled_ = false;
					break;
				}
		}
	};

	template<typename Graph>
	struct VertexPredicate
	{
		Graph *G_;

		VertexPredicate ()
		: G_ (0)
		{
		}

		VertexPredicate (Graph& g)
		: G_ (&g)
		{
		}

		template<typename Vertex>
		bool operator() (const Vertex& u) const
		{
			return (*G_) [u].IsFulfilled_;
		}
	};

	void PluginTreeBuilder::Calculate ()
	{
		Graph_.clear ();
		Object2Vertex_.clear ();
		Result_.clear ();

		CreateGraph ();
		const auto& edge2vert = MakeEdges ();

		QMap<Vertex_t, QList<Vertex_t>> reachable;
		for (const auto& pair : edge2vert)
			reachable [pair.first] << pair.second;

		QList<Edge_t> backEdges;
		CycleDetector<Edge_t> cd (backEdges);
		boost::depth_first_search (Graph_, boost::visitor (cd));

		QList<Vertex_t> backVertices;
		for (const auto& backEdge : backEdges)
			backVertices << edge2vert [backEdge].first;

		FulfillableChecker<Graph_t, Vertex_t> checker (Graph_, backVertices, reachable);
		boost::depth_first_search (Graph_, boost::visitor (checker));

		boost::filtered_graph<Graph_t, boost::keep_all, VertexPredicate<Graph_t>> fulfilledSubgraph
		{
			Graph_,
			boost::keep_all {},
			VertexPredicate<Graph_t> (Graph_)
		};

		QList<Vertex_t> vertices;
		boost::topological_sort (fulfilledSubgraph, std::back_inserter (vertices));
		for (const auto& vertex : vertices)
			Result_ << fulfilledSubgraph [vertex].Object_;
	}

	QObjectList PluginTreeBuilder::GetResult () const
	{
		return Result_;
	}

	void PluginTreeBuilder::CreateGraph ()
	{
		for (const auto object : Instances_)
		{
			try
			{
				VertexInfo vi (object);
				Vertex_t objVertex = boost::add_vertex (Graph_);
				Graph_ [objVertex] = vi;
				Object2Vertex_ [object] = objVertex;
			}
			catch (const std::exception& e)
			{
				qWarning () << Q_FUNC_INFO
						<< e.what ()
						<< "for"
						<< object
						<< "; skipping";
				continue;
			}
		}
	}

	QMap<PluginTreeBuilder::Edge_t, QPair<PluginTreeBuilder::Vertex_t, PluginTreeBuilder::Vertex_t>> PluginTreeBuilder::MakeEdges ()
	{
		QSet<QPair<Vertex_t, Vertex_t>> depVertices;
		boost::graph_traits<Graph_t>::vertex_iterator vi, vi_end;
		for (boost::tie (vi, vi_end) = boost::vertices (Graph_); vi != vi_end; ++vi)
		{
			const QSet<QString>& fdeps = Graph_ [*vi].UnfulfilledFeatureDeps_;
			const QSet<QByteArray>& pdeps = Graph_ [*vi].UnfulfilledP2PDeps_;
			if (!fdeps.size () && !pdeps.size ())
				continue;

			boost::graph_traits<Graph_t>::vertex_iterator vj, vj_end;
			for (boost::tie (vj, vj_end) = boost::vertices (Graph_); vj != vj_end; ++vj)
			{
				const QSet<QString> fInter = QSet<QString> (fdeps).intersect (Graph_ [*vj].FeatureProvides_);
				if (fInter.size ())
				{
					Graph_ [*vi].UnfulfilledFeatureDeps_.subtract (fInter);
					depVertices << qMakePair (*vi, *vj);
				}

				const QSet<QByteArray> pInter = QSet<QByteArray> (pdeps).intersect (Graph_ [*vj].P2PProvides_);
				if (pInter.size ())
				{
					Graph_ [*vi].UnfulfilledP2PDeps_.subtract (pInter);
					depVertices << qMakePair (*vi, *vj);
				}
			}
		}

		QMap<Edge_t, QPair<Vertex_t, Vertex_t>> result;
		for (const auto& pair : depVertices)
			result [boost::add_edge (pair.first, pair.second, Graph_).first] = pair;
		return result;
	}
}
