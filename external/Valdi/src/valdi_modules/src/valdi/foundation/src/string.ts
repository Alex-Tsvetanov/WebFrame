/**
 * Given a URL path returns a slugified path.
 *
 * ie: '/rpc/search' becomes 'rpcsearch'
 *
 * ie:
 * /rpc/snapchat.premiumstories.PremiumStories/GetStory?story_id=577361270638592&profile=1
 * becomes
 * rpcsnapchatpremiumstoriesPremiumStoriesGetStory
 */
export function slugifyUrlPath(path: string): string {
  return path
    .replace(/\?.*/g, '') // strip url parameters
    .replace(/[^a-z0-9 -]/gi, '') // remove invalid chars
    .replace(/\s+/g, '-') // collapse whitespace and replace by -
    .replace(/-+/g, '-'); // collapse dashes
}
