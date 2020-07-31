/**********************************************************************
 * LeechCraft - modular cross-platform feature rich internet client.
 * Copyright (C) 2006-2014  Georg Rudoy
 *
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE or copy at https://www.boost.org/LICENSE_1_0.txt)
 **********************************************************************/

#ifndef INTERFACES_CORE_ITAGSMANAGER_H
#define INTERFACES_CORE_ITAGSMANAGER_H
#include <QStringList>

class QAbstractItemModel;

/** @brief Tags manager's interface.
 *
 * This interface is for communication with the tags manager.
 *
 * Object returned by the GetQObject() function emits these signals:
 * - tagsUpdated(const QStringList& tags) when the tags are updated.
 */
class Q_DECL_EXPORT ITagsManager
{
public:
	typedef QString tag_id;

	virtual ~ITagsManager () {}

	/** @brief Returns the ID of the given \em tag.
	 *
	 * If there is no such tag, it's added to the tag collection and the
	 * id of the new tag is returned.
	 *
	 * @param[in] tag The tag that should be identified.
	 * @return The ID of the tag.
	 *
	 * @sa GetTag()
	 * @sa GetIDs()
	 */
	virtual tag_id GetID (const QString& tag) = 0;

	/** @brief Returns the IDs of the given \em tags.
	 *
	 * This convenience function invokes GetID() for each tag in \em tags
	 * and returns the list of the corresponding tag IDs.
	 *
	 * @param[in] tags The tags that should be identified.
	 * @return The IDs of the tags.
	 *
	 * @sa GetID()
	 */
	QList<tag_id> GetIDs (const QStringList& tags)
	{
		QList<tag_id> result;
		for (const auto& tag : tags)
			result << GetID (tag);
		return result;
	}

	/** @brief Returns the tag with the given \em id.
	 *
	 * If there is no such tag, a null QString is returned. A sensible
	 * plugin would delete the given id from the list of assigned tags
	 * for all the items with this id.
	 *
	 * @param[in] id The id of the tag.
	 * @return The tag.
	 *
	 * @sa GetID()
	 * @sa GetTags()
	 */
	virtual QString GetTag (tag_id id) const = 0;

	/** @brief Returns the tags with the given \em ids.
	 *
	 * This convenience function invokes GetTag() for each tag ID in
	 * \em ids and returns the list of the corresponding tags.
	 *
	 * @param[in] ids The ids of the tags.
	 * @return The tags corresponding to the \em ids.
	 *
	 * @sa GetTag()
	 */
	QStringList GetTags (const QList<tag_id>& ids) const
	{
		QStringList result;
		for (const auto& id : ids)
			result << GetTag (id);
		return result;
	}

	/** @brief Returns all tags existing in LeechCraft now.
	 *
	 * @return List of all tags.
	 */
	virtual QStringList GetAllTags () const = 0;

	/** @brief Splits the given string with tags to the list of the tags.
	 *
	 * @param[in] string String with tags.
	 * @return The list of the tags.
	 */
	virtual QStringList Split (const QString& string) const = 0;

	/** @brief Splits the given string with tags and returns tags IDs.
	 *
	 * This function is essentially a combination of Split() and GetID().
	 * First, the given string is split into human-readable tags, and
	 * then for each tag its ID is obtained.
	 *
	 * @param[in] string String with tags.
	 * @return The list of the tags IDs.
	 */
	virtual QList<tag_id> SplitToIDs (const QString& string) = 0;

	/** @brief Joins the given tags into one string that's suitable to
	 * display to the user.
	 *
	 * @param[in] tags List of tags.
	 * @return The joined string with tags.
	 */
	virtual QString Join (const QStringList& tags) const = 0;

	/** @brief Joins the given tag IDs into one human-readable string.
	 *
	 * This function is essentially a combination of GetTag() and Join().
	 * First, it converts all given tagIDs into tag names using GetTag()
	 * and then joins them using Join(). This function is provided for
	 * convenience.
	 *
	 * @param[in] tagIDs List of tag IDs.
	 * @return The joined string with tags.
	 */
	virtual QString JoinIDs (const QStringList& tagIDs) const = 0;

	/** @brief Returns the completion model for this tags manager.
	 *
	 * The returned completion model can be used in a QCompleter class.
	 * It uses the tags from this tags manager to provide completions as
	 * the user types.
	 *
	 * @return The completion model suitable for usage in a QCompleter.
	 */
	virtual QAbstractItemModel* GetModel () = 0;

	/** @brief Returns the tags manager as a QObject to get access to
	 * all the meta-stuff.
	 *
	 * @return This object as a QObject.
	 */
	virtual QObject* GetQObject () = 0;
};

Q_DECLARE_INTERFACE (ITagsManager, "org.Deviant.LeechCraft.ITagsManager/1.0")

#endif
