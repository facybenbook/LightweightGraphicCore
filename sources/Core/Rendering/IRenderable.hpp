namespace LWGE
{
	namespace Core
	{
		class IRenderable
		{
			public:
				void			OnPreRender(void) = default;
				void			Render(void) = 0;
				void			OnPostRender(void) = default;

				SortingLayer	GetSortingLayer(void) = 0;
				Bounds			GetBounds(void) = 0;
		}
	}
}